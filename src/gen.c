#include "gen.h"

#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>

#include "util/util.h"

#define Imm(value) \
    (operand) { .Type = OPERAND_IMM, .Imm.Value = (value) }

#define Reg(reg, size) \
    (operand) { .Type = OPERAND_REG, .Size = size, .Reg.Register = reg }
#define Rbp (Reg(REG_RBP, 8))
#define Rsp (Reg(REG_RSP, 8))

#define ConstDisplacement(disp) \
    (displacement) { .Type = DISPLACEMENT_CONSTANT, .Value = (disp) }
#define LabelDisplacement(label) \
    (displacement) { .Type = DISPLACEMENT_LABEL, .Label = (label) }

constexpr register_id ParamRegisters[]   = {REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9};
constexpr register_id ScratchRegisters[] = {REG_RBX, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15};

int NextFreeRegister        = 0;
int MaxScratchRegistersUsed = 0;

void emit_error(const char *format, ...) {
    va_list Args;
    va_start(Args, format);

    printf("[Codegen Error] ");

    vprintf(format, Args);
    printf("\n");

    va_end(Args);
}

operand LabelOperand(string Label) { return (operand){.Type = OPERAND_LABEL, .Label.Name = Label}; }

void emit_count(program_code *code, asm_instruction *instructions, int count) {
    asm_instruction *Ptr = arena_push(&code->InstructionArena, count * sizeof(asm_instruction));

    // Debug
    /*
    for (int i = 0; i < count; i++) {
        asm_instruction Instr = instructions[i];

        if (Instr.Op == ASM_MOV && Instr.Dst.Type == OPERAND_REG && Instr.Dst.Reg.Register == REG_R10) {
            raise(SIGTRAP);
        }
    }
    */

    if (Ptr) {
        memcpy(Ptr, instructions, count * sizeof(asm_instruction));
        code->InstructionCount += count;
    }
}

void emit(program_code *code, asm_instruction instruction) { emit_count(code, &instruction, 1); }

void emit_nop(program_code *code) { emit(code, (asm_instruction){.Op = ASM_NOP}); }

string new_label(program_code *code) {
    string Name = string_make(code->GeneralArena, 32);

    Name.Data[0] = 'L';
    Name.Length++;

    Name.Length += sprintf(Name.Data + 1, "%d", code->Label++);

    return Name;
}

string emit_label(program_code *code, string Name) {
    emit(code, (asm_instruction){.Op = ASM_LABEL, .Dst = (operand){.Label.Name = Name}});
    return Name;
}

string emit_next_label(program_code *code) { return emit_label(code, new_label(code)); }

string emit_function_label(program_code *code, string Name) {
    emit(code, (asm_instruction){.Op = ASM_LABEL, .Dst = (operand){.Label.Name = Name}});
    return Name;
}

void push_loop(program_code *code, string BreakLabel, string ContinueLabel) {
    loop Loop = {BreakLabel, ContinueLabel};

    code->Loops[code->LoopCount++] = Loop;
}

void pop_loop(program_code *code) {
    if (code->LoopCount == 0) return;

    code->LoopCount--;
}

void emit_spill_params(program_code *code, symbol *FuncSym, size_t local_size) {
    for (int i = 0; i < FuncSym->Function.ParamCount; i++) {
        symbol *Param = FuncSym->Function.Params[i];

        // Give each parameter a stack slot
        local_size += Param->TypeInfo.Size;

        Param->StackOffset = local_size;
        Param->Section     = SECTION_STACK;

        operand Dst = {
            .Type = OPERAND_MEM,
            .Size = Param->TypeInfo.Size,
            .Mem  = {.Base = REG_RBP, .Displacement = ConstDisplacement(-Param->StackOffset)}
        };

        emit_move(code, Dst, Reg(ParamRegisters[i], Param->TypeInfo.Size));
    }
}

size_t total_param_bytes(symbol *FuncSym) {
    size_t Result = 0;

    for (int i = 0; i < FuncSym->Function.ParamCount; i++) {
        Result += FuncSym->Function.Params[i]->TypeInfo.Size;
    }

    return Result;
}

asm_instruction *emit_function_prologue(program_code *code, asm_instruction **SubRspInstruction) {
    asm_instruction Instructions[] = {
        {.Op = ASM_PUSH, .Dst = Rbp},
        {.Op = ASM_MOV, .Dst = Rbp, .Src = Rsp},
        {.Op = ASM_SUB, .Dst = Rsp, .Src = Imm(0)},
    };

    int Count = ArraySize(Instructions);

    *SubRspInstruction = (asm_instruction *)code->InstructionArena.Data + code->InstructionCount + 2;

    emit_count(code, Instructions, Count);

    asm_instruction *Result = ((asm_instruction *)code->InstructionArena.Data) + code->InstructionCount;

    asm_instruction ScratchPushes[ArraySize(ScratchRegisters)] = {};
    emit_count(code, ScratchPushes, ArraySize(ScratchPushes));

    return Result;
}

string function_end_label(program_code *code) {
    string FunctionName = code->CurrentFunction;

    string Name = string_make(code->GeneralArena, FunctionName.Length + 6);
    Name.Length = sprintf(Name.Data, "Lend_%.*s", (int)FunctionName.Length, FunctionName.Data);
    return Name;
}

// Searches if the entry already exists, and returns that. Otherwise Returns entry label
string push_rodata_entry(program_code *code, rodata_entry Entry) {
    if (Entry.Type == RODATA_STRING_LIT) {
        for (int i = 0; i < code->RodataEntryCount; i++) {
            rodata_entry *E = code->RodataEntries + i;

            if (E->Type == RODATA_STRING_LIT && string_equals(Entry.StringLit, E->StringLit)) {
                return E->Label;
            }
        }
    }

    string Name = string_make(code->GeneralArena, 8);
    Name.Length = sprintf(Name.Data, "LS%d", ++code->RodataEntryCount);

    Entry.Label = Name;

    code->RodataEntries[code->RodataEntryCount - 1] = Entry;

    return Name;
}

void emit_program_epilogue(program_code *code) {
    asm_instruction Instructions[] = {
        {.Op = ASM_MOV, .Dst = Reg(REG_RDI, 8), .Src = Reg(REG_RAX, 8)},
        {.Op = ASM_MOV, .Dst = Reg(REG_RAX, 8), .Src = Imm(60)},
        {.Op = ASM_SYSCALL},
    };

    emit_label(code, function_end_label(code));
    emit_count(code, Instructions, ArraySize(Instructions));
}

void emit_function_epilogue(program_code *code) {
    asm_instruction Instructions[] = {
        {.Op = ASM_MOV, .Dst = Rsp, .Src = Rbp},
        {.Op = ASM_POP, .Dst = Rbp},
        {.Op = ASM_RET},
    };

    emit_label(code, function_end_label(code));
    emit_free_all_scratch_registers(code);
    emit_count(code, Instructions, ArraySize(Instructions));
}

operand emit_and(program_code *code, operand Left, operand Right) {
    if (Left.Type == OPERAND_IMM && Right.Type == OPERAND_IMM) {
        return Imm(Left.Imm.Value && Right.Imm.Value);
    }

    string TrueEnd  = new_label(code);
    string FalseEnd = new_label(code);

    emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Left, .Src = Imm(0)});
    emit(code, (asm_instruction){.Op = ASM_JE, .Dst = LabelOperand(FalseEnd)});

    emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Right, .Src = Imm(0)});
    emit(code, (asm_instruction){.Op = ASM_JE, .Dst = LabelOperand(FalseEnd)});

    emit_move(code, Reg(REG_RAX, 1), Imm(1));
    emit(code, (asm_instruction){.Op = ASM_JMP, .Dst = LabelOperand(TrueEnd)});

    emit_label(code, FalseEnd);
    emit_move(code, Reg(REG_RAX, 1), Imm(0));

    emit_label(code, TrueEnd);

    return Reg(REG_RAX, 1);
}

operand emit_or(program_code *code, operand Left, operand Right) {
    if (Left.Type == OPERAND_IMM && Right.Type == OPERAND_IMM) {
        return Imm(Left.Imm.Value || Right.Imm.Value);
    }

    string TrueEnd = new_label(code);
    string End     = new_label(code);

    emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Left, .Src = Imm(0)});
    emit(code, (asm_instruction){.Op = ASM_JNE, .Dst = LabelOperand(TrueEnd)});

    emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Right, .Src = Imm(0)});
    emit(code, (asm_instruction){.Op = ASM_JNE, .Dst = LabelOperand(TrueEnd)});

    emit_move(code, Reg(REG_RAX, 1), Imm(0));
    emit(code, (asm_instruction){.Op = ASM_JMP, .Dst = LabelOperand(End)});

    emit_label(code, TrueEnd);
    emit_move(code, Reg(REG_RAX, 1), Imm(1));

    emit_label(code, End);
    return Reg(REG_RAX, 1);
}

void emit_jump(program_code *code, string Label) {
    emit(code, (asm_instruction){.Op = ASM_JMP, .Dst = LabelOperand(Label)});
}

void emit_cmp(program_code *code, operand Left, operand Right) {
    if (Left.Size == 0)
        Left.Size = Right.Size;
    else if (Right.Size == 0)
        Right.Size = Left.Size;

    if (Left.Size < Right.Size) {
        Right.Size = Left.Size;
    }

    if (Left.Type == OPERAND_MEM && Right.Type == OPERAND_MEM) {
        operand Tmp = scratch_register(Left.Size);
        emit_move(code, Tmp, Left);
        emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Tmp, .Src = Right});
        free_scratch_register();
    } else {
        emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Left, .Src = Right});
    }
}

void emit_test(program_code *code, operand A, operand B) {
    operand Tmp;
    bool UsedScratch = false;

    if (A.Type != OPERAND_REG) {
        Tmp = scratch_register(A.Size);
        emit_move(code, Tmp, A);
        A           = Tmp;
        B           = Tmp;
        UsedScratch = true;
    }

    emit(code, (asm_instruction){.Op = ASM_TEST, .Dst = A, .Src = B});

    if (UsedScratch) free_scratch_register();
}

void emit_je(program_code *code, string Label) {
    emit(code, (asm_instruction){.Op = ASM_JE, .Dst = LabelOperand(Label)});
}

operand emit_comparison(program_code *code, token_type Op, operand Left, operand Right) {
    emit_cmp(code, Left, Right);

    asm_opcode SetOpcode;

    switch (Op) {
        case TOKEN_LESS:        SetOpcode = ASM_SETL; break;
        case TOKEN_MORE:        SetOpcode = ASM_SETG; break;
        case TOKEN_LESS_EQUALS: SetOpcode = ASM_SETLE; break;
        case TOKEN_MORE_EQUALS: SetOpcode = ASM_SETGE; break;
        default:                assert(false); break;
    }

    emit(code, (asm_instruction){.Op = SetOpcode, .Dst = Reg(REG_RAX, 1)});
    emit(code, (asm_instruction){.Op = ASM_MOVZX, .Dst = Reg(REG_RAX, 4), .Src = Reg(REG_RAX, 1)});

    return Reg(REG_RAX, 4);
}

operand get_string_lit_operand(program_code *code, string Label) {
    return (operand){
        .Type = OPERAND_MEM,
        .Size = 8,
        .Mem  = {.IsAddress = true, .Base = REG_RIP, .Displacement = LabelDisplacement(Label)}
    };
}

void emit_lea(program_code *code, operand DstReg, operand SrcMem) {
    assert(DstReg.Type == OPERAND_REG);
    assert(SrcMem.Type == OPERAND_MEM);

    SrcMem.Size = 0;
    DstReg.Size = 8;

    emit(code, (asm_instruction){.Op = ASM_LEA, .Dst = DstReg, .Src = SrcMem});
}

operand emit_math(program_code *code, token_type Op, operand Left, operand Right) {
    int Size = Max(Left.Size, Right.Size);

    if (Size == 0) Size = 4;

    asm_opcode MathOp;

    switch (Op) {
        case TOKEN_PLUS:   MathOp = ASM_ADD; break;
        case TOKEN_MINUS:  MathOp = ASM_SUB; break;
        case TOKEN_STAR:   MathOp = ASM_IMUL; break;
        case TOKEN_DIVIDE: MathOp = ASM_IDIV; break;
        default:           assert(false); break;
    }

    if (Op == TOKEN_PLUS || Op == TOKEN_MINUS) {
        if (Left.Type == OPERAND_IMM && Left.Imm.Value == 0) return (operand){};
        if (Right.Type == OPERAND_IMM && Right.Imm.Value == 0) return (operand){};
    }

    operand Tmp;
    bool UsedScratch = false;

    if (MathOp == ASM_IDIV) {
        emit_move(code, Reg(REG_RAX, Size), Left);

        // Why did we do this again...?
        emit(code, (asm_instruction){.Op = ASM_CDQ});

        if (Right.Type == OPERAND_IMM) {
            Tmp = scratch_register(Size);
            emit_move(code, Tmp, Right);
            Right       = Tmp;
            UsedScratch = true;
        }

        emit(code, (asm_instruction){.Op = ASM_IDIV, .Dst = Right});
    } else {
        if (Right.Type == OPERAND_REG && Right.Reg.Register == REG_RAX) {
            Tmp = scratch_register(Size);
            emit_move(code, Tmp, Right);
            Right       = Tmp;
            UsedScratch = true;
        }
        emit_move(code, Reg(REG_RAX, Size), Left);
        emit(code, (asm_instruction){.Op = MathOp, .Dst = Reg(REG_RAX, Size), .Src = Right});
    }

    if (UsedScratch) {
        free_scratch_register();
    }

    return Reg(REG_RAX, Size);
}

bool expr_may_clobber_rax(ast_node *node) {
    switch (node->Type) {
        case NODE_IDENT:
        case NODE_INT_LIT:
        case NODE_CHAR_LIT:
        case NODE_STRING_LIT: return false;
        default:              return true;
    }
}

// The result is stored in the register that the return value points to (Rax)
operand emit_binop(ast_node *node, program_code *code) {
    token_type Op = node->BinaryOp.Operation;

    operand Left = emit_expression(node->BinaryOp.Left, code);

    operand SafeLeft;
    bool UsedScratch = false;

    if (Left.Type == OPERAND_REG && Left.Reg.Register == REG_RAX && expr_may_clobber_rax(node->BinaryOp.Right)) {
        SafeLeft = scratch_register(Left.Size);
        emit_move(code, SafeLeft, Left);
        Left = SafeLeft;

        UsedScratch = true;
    }

    operand Right = emit_expression(node->BinaryOp.Right, code);

    operand Result;

    switch (Op) {
        case TOKEN_EQUALS_EQUALS:
        case TOKEN_BANG_EQUALS:   {
            emit_cmp(code, Left, Right);

            asm_opcode Set = Op == TOKEN_EQUALS_EQUALS ? ASM_SETE : ASM_SETNE;

            emit(code, (asm_instruction){.Op = Set, .Dst = Reg(REG_RAX, 1)});

            emit(code, (asm_instruction){.Op = ASM_MOVZX, .Dst = Reg(REG_RAX, 4), .Src = Reg(REG_RAX, 1)});

            Result = Reg(REG_RAX, 4);
            break;
        }
        case TOKEN_LESS:
        case TOKEN_MORE:
        case TOKEN_LESS_EQUALS:
        case TOKEN_MORE_EQUALS: {
            Result = emit_comparison(code, Op, Left, Right);
            break;
        }
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            if (is_pointer_math_op(code, node)) {
                Result = emit_pointer_math_op(code, node, REG_RAX);
                break;
            }
        case TOKEN_STAR:
        case TOKEN_DIVIDE: {
            Result = emit_math(code, node->BinaryOp.Operation, Left, Right);
            break;
        }

        case TOKEN_OPEN_SQUARE: {
            Result = emit_dereference(code, node, REG_RAX);
            break;
        }

        case TOKEN_AND: {
            Result = emit_and(code, Left, Right);
            break;
        }

        case TOKEN_OR: {
            Result = emit_or(code, Left, Right);
            break;
        }

        default: {
            emit_error("Didn't implement this binary operation %s\n", token_name(Op));
            Result = (operand){};
            break;
        }
    }

    if (UsedScratch) free_scratch_register();

    return Result;
}

operand emit_negate(program_code *code, ast_node *node, operand Operand) {
    if (Operand.Type == OPERAND_IMM) {
        assert(node->UnaryOp.Operand->Type == NODE_INT_LIT);
        Operand.Imm.Value = -node->UnaryOp.Operand->IntegerLit.Value;
    } else if (Operand.Type == OPERAND_REG || Operand.Type == OPERAND_MEM) {
        emit(code, (asm_instruction){.Op = ASM_NEG, .Dst = Operand});
    } else {
        assert(false);
    }

    return Operand;
}

operand emit_not(program_code *code, operand Operand) {
    if (Operand.Type == OPERAND_IMM) {
        if (Operand.Imm.Value == 0) {
            return Imm(1);
        } else {
            return Imm(0);
        }
    } else if (Operand.Type == OPERAND_REG || Operand.Type == OPERAND_MEM) {
        operand Rax = Reg(REG_RAX, Operand.Size);
        operand Al  = Reg(REG_RAX, 1);

        emit_move(code, Rax, Operand);
        emit_test(code, Rax, Rax);
        emit(code, (asm_instruction){.Op = ASM_SETE, Al});

        if (Rax.Size > Al.Size) emit(code, (asm_instruction){.Op = ASM_MOVZX, Rax, Al});

        return Rax;
    }

    assert(false);
    return Reg(REG_RAX, Operand.Size);
}

operand emit_inc_dec(program_code *Code, ast_node *Node, bool Increment, bool Prefix) {
    assert(Node->Type == NODE_UNARY_OP);

    operand Var = emit_expression(Node->UnaryOp.Operand, Code);

    if (Var.Type == OPERAND_IMM) {
        emit_error("Can't ++ or -- an immediate value.");
        return (operand){};
    }

    if (!Prefix) {
        emit_move(Code, Reg(REG_RAX, Var.Size), Var);
    }

    asm_opcode Op = Increment ? ASM_ADD : ASM_SUB;
    emit(Code, (asm_instruction){.Op = Op, .Dst = Var, .Src = Imm(1)});

    return Prefix ? Var : Reg(REG_RAX, Var.Size);
}

// Get type info of a node
//   IF Principal IS TRUE:
//     Say *char a;
//     get_type_info_from_operand(++a) => type info for *char
//     get_type_info_from_operand(a + 2) => type info for *char
//     get_type_info_from_operand(a[0]) => type info for char
//   IF Principal IS FALSE:
//     Say *char a;
//     get_type_info_from_operand(a[0]) => type info for *char
type_info get_type_info_from_operand(ast_node *Node, bool Principal) {
    type_info Result = {};

    // If Principal is false, we would need to return PointingTo's type_info
    bool IsDereferencing = false;

    switch (Node->Type) {
        case NODE_IDENT: {
            Result = Node->Ident.Sym->TypeInfo;
            break;
        }
        case NODE_BINARY_OP: {
            token_type Op = Node->BinaryOp.Operation;

            if (Op != TOKEN_OPEN_SQUARE && Op != TOKEN_PLUS && Op != TOKEN_MINUS) return Result;

            IsDereferencing = (Op == TOKEN_OPEN_SQUARE);

            type_info A = get_type_info_from_operand(Node->BinaryOp.Left, Principal);
            type_info B = get_type_info_from_operand(Node->BinaryOp.Right, Principal);

            if (A.IndirectionDepth > 0 && B.IndirectionDepth > 0) {
                emit_error("Pointer math can only be done with one pointer operand.");
            } else if (A.IndirectionDepth > 0) {
                Result = A;

            } else {
                Result = B;
            }
            break;
        }
        case NODE_UNARY_OP: {
            Result          = get_type_info_from_operand(Node->UnaryOp.Operand, Principal);
            IsDereferencing = (Node->UnaryOp.Operation == TOKEN_STAR);
            break;
        }
        default: break;
    }

    if (!Principal && IsDereferencing) {
        assert(Result.PointingTo);
        Result = *Result.PointingTo;
    }

    return Result;
}

operand emit_sizeof(program_code *code, ast_node *Operand) {
    type_info TypeInfo = get_type_info_from_operand(Operand, false);
    return Imm(TypeInfo.Size);
}

bool is_pointer_math_op(program_code *code, ast_node *Node) {
    if (Node->Type != NODE_BINARY_OP) return false;

    type_info A = get_type_info_from_operand(Node->BinaryOp.Left, true);
    type_info B = get_type_info_from_operand(Node->BinaryOp.Right, true);

    if (A.IndirectionDepth > 0 && B.IndirectionDepth > 0) return false;
    if (A.IndirectionDepth > 0 || B.IndirectionDepth > 0) return true;

    return false;
}

operand access_array(program_code *code,
                     operand Left,
                     operand Right,
                     bool Subtract,
                     type_info PtrTypeInfo,
                     type_info TypeInfoLeft,
                     register_id OutputReg) {
    assert(PtrTypeInfo.IndirectionDepth > 0);
    assert(PtrTypeInfo.PointingTo);

    operand Ptr = TypeInfoLeft.IndirectionDepth > 0 ? Left : Right;
    operand Off = TypeInfoLeft.IndirectionDepth > 0 ? Right : Left;

    int ElemSize = PtrTypeInfo.PointingTo->Size;

    if (Off.Type == OPERAND_IMM) {
        int Disp = Off.Imm.Value * ElemSize;

        if (Subtract) Disp *= -1;

        emit_move(code, Reg(OutputReg, 8), Ptr);

        operand Mem = {
            .Type = OPERAND_MEM,
            .Mem  = {.Base = OutputReg, .Displacement = {.Type = DISPLACEMENT_CONSTANT, .Value = Disp}}
        };

        emit_lea(code, Reg(OutputReg, 8), Mem);

        return Reg(OutputReg, 8);
    } else if (Off.Type == OPERAND_REG && (ElemSize == 1 || ElemSize == 2 || ElemSize == 4 || ElemSize == 8)) {
        if (Subtract) emit(code, (asm_instruction){.Op = ASM_NEG, .Dst = Off});

        if (Off.Reg.Register == OutputReg) {
            operand Scratch = scratch_register(8);  // increase to 64-bit

            if (Off.Size < 8)
                emit(code, (asm_instruction){.Op = ASM_MOVSX, .Dst = Scratch, .Src = Off});
            else
                emit_move(code, Scratch, Off);

            emit_move(code, Reg(OutputReg, 8), Ptr);

            operand Mem = {
                .Type = OPERAND_MEM, .Mem = {.Base = OutputReg, .Index = Scratch.Reg.Register, .Scale = ElemSize}
            };

            emit_lea(code, Reg(OutputReg, 8), Mem);

            free_scratch_register();
        } else {
            emit_move(code, Reg(OutputReg, 8), Ptr);

            emit_lea(
                code,
                Reg(OutputReg, 8),
                (operand){
                    .Type = OPERAND_MEM, .Mem = {.Base = OutputReg, .Index = Off.Reg.Register, .Scale = ElemSize}
            });
        }

        return Reg(OutputReg, 8);
    } else {
        assert(Off.Type == OPERAND_MEM);

        asm_opcode Opcode = Subtract ? ASM_SUB : ASM_ADD;

        // (imul) Multiply offset by ElemSize
        if (ElemSize > 1) {
            Off = emit_math(code, TOKEN_STAR, Off, Imm(ElemSize));

            // move the result out from OutputReg, since we need OutputReg to store the ptr
            operand Scratch = scratch_register(8);  // increase to 64-bit

            if (Off.Size < 8)
                emit(code, (asm_instruction){.Op = ASM_MOVSX, .Dst = Scratch, .Src = Off});
            else
                emit_move(code, Scratch, Off);

            emit_move(code, Reg(OutputReg, 8), Ptr);

            emit(code, (asm_instruction){.Op = Opcode, .Dst = Reg(OutputReg, 8), .Src = Scratch});

            free_scratch_register();

            return Reg(OutputReg, 8);
        } else {
            emit_move(code, Reg(OutputReg, 8), Ptr);

            if (Off.Size < 8) {
                operand Scratch = scratch_register(8);
                emit(code, (asm_instruction){.Op = ASM_MOVSX, .Dst = Scratch, .Src = Off});
                emit(code, (asm_instruction){.Op = Opcode, .Dst = Reg(OutputReg, 8), .Src = Scratch});
                free_scratch_register();
            } else {
                emit(code, (asm_instruction){.Op = Opcode, .Dst = Reg(OutputReg, 8), .Src = Off});
            }

            return Reg(OutputReg, 8);
        }
    }
}

// Returns pointer into OutputReg
operand emit_pointer_math_op(program_code *code, ast_node *Operand, register_id OutputReg) {
    type_info TypeInfo = get_type_info_from_operand(Operand, true);

    switch (Operand->Type) {
        case NODE_IDENT: {
            operand Mem = emit_expression(Operand, code);

            // For arrays, we have to do a lea, not just a mov
            if (Operand->Ident.Sym->TypeInfo.IsArray) Mem.Mem.IsAddress = true;

            emit_move(code, Reg(OutputReg, 8), Mem);
            return Reg(OutputReg, 8);
        }
        case NODE_BINARY_OP: {
            token_type Operation = Operand->BinaryOp.Operation;

            switch (Operation) {
                case TOKEN_PLUS:
                case TOKEN_MINUS:
                case TOKEN_OPEN_SQUARE: {
                    operand Left  = emit_expression(Operand->BinaryOp.Left, code);
                    operand Right = emit_expression(Operand->BinaryOp.Right, code);

                    type_info TypeInfoLeft = get_type_info_from_operand(Operand->BinaryOp.Left, false);

                    return access_array(code, Left, Right, Operation == TOKEN_MINUS, TypeInfo, TypeInfoLeft, OutputReg);
                }
                default: {
                    // TODO: Error message
                    assert(false);
                    break;
                }
            }
            break;
        }
        case NODE_UNARY_OP: {
            token_type Operation = Operand->UnaryOp.Operation;

            switch (Operation) {
                case TOKEN_INC: {
                    return emit_inc_dec(code, Operand, true, Operand->UnaryOp.First);
                }
                case TOKEN_DEC: {
                    return emit_inc_dec(code, Operand, false, Operand->UnaryOp.First);
                }
                case TOKEN_SIZEOF: {
                    return emit_sizeof(code, Operand->UnaryOp.Operand);
                }
                default: {
                    break;
                }
            }
        }
        default: {
            // TODO: Error message
            printf("Got %d\n", Operand->Type);
            assert(false);
            break;
        }
    }

    assert(false);

    return (operand){};
}

operand emit_dereference(program_code *code, ast_node *OperandNode, register_id OutputReg) {
    assert((OperandNode->Type == NODE_UNARY_OP && OperandNode->UnaryOp.Operation == TOKEN_STAR) ||
           (OperandNode->Type == NODE_BINARY_OP && OperandNode->BinaryOp.Operation == TOKEN_OPEN_SQUARE));

    bool IsUnary = OperandNode->Type == NODE_UNARY_OP;

    // Extract type info to get size
    type_info TypeInfo = get_type_info_from_operand(OperandNode, true);

    ast_node *Operand = IsUnary ? OperandNode->UnaryOp.Operand : OperandNode;

    emit_pointer_math_op(code, Operand, OutputReg);

    if (TypeInfo.PointingTo->Size <= 8) {
        operand Dst = {};

        Dst.Type          = OPERAND_MEM;
        Dst.Size          = TypeInfo.PointingTo->Size;
        Dst.Mem.IsAddress = false;
        Dst.Mem.Base      = OutputReg;

        return Dst;
    } else {
        // TODO: Emit memcpy?
        assert(false);
    }

    return Reg(OutputReg, 8);
}

operand emit_addressof(program_code *code, ast_node *OperandNode) {
    assert(OperandNode->Type == NODE_UNARY_OP);
    assert(OperandNode->UnaryOp.Operation == TOKEN_AMP);

    operand Op = emit_expression(OperandNode->UnaryOp.Operand, code);
    assert(Op.Type == OPERAND_MEM);
    emit_lea(code, Reg(REG_RAX, 8), Op);
    return Reg(REG_RAX, 8);
}

operand emit_unaryop(ast_node *node, program_code *code) {
    token_type Operation = node->UnaryOp.Operation;

    if (Operation == TOKEN_STAR) {
        return emit_dereference(code, node, REG_RAX);
    }

    if (Operation == TOKEN_AMP) {
        return emit_addressof(code, node);
    }

    operand Operand;

    if (Operation == TOKEN_INC || Operation == TOKEN_DEC || Operation == TOKEN_SIZEOF) {
        // We are good, no need to emit anything.
    } else {
        Operand = emit_expression(node->UnaryOp.Operand, code);
    }

    bool Prefix = node->UnaryOp.First;

    switch (Operation) {
        case TOKEN_MINUS:  return emit_negate(code, node, Operand);
        case TOKEN_INC:    return emit_inc_dec(code, node, true, Prefix);
        case TOKEN_DEC:    return emit_inc_dec(code, node, false, Prefix);
        case TOKEN_BANG:   return emit_not(code, Operand);
        case TOKEN_SIZEOF: return emit_sizeof(code, node);
        default:           assert(false); break;
    }

    return Operand;
}

operand emit_call(ast_node *node, program_code *code) {
    assert(node->Type == NODE_CALL);

    string Name = node->Call.FuncName->Ident.Name;

    if (node->Call.ArgCount > ArraySize(ParamRegisters)) {
        emit_error("Haven't gotten to implementing stack arguments yet!");
        assert(false);
    }

    func_data FuncData = node->Call.FuncName->Ident.Sym->Function;
    bool IsVariadic    = FuncData.IsVariadic;

    printf("%.*s %d\n", (int)node->FuncDef.Name->Ident.Name.Length, node->FuncDef.Name->Ident.Name.Data, IsVariadic);

    const int FloatCount = 0;

    for (int i = node->Call.ArgCount - 1; i >= 0; i--) {
        int ExpectedArgSize;
        operand ArgI = emit_expression(node->Call.Args[i], code);

        // Variadic?
        if (i >= FuncData.ParamCount) {
            ExpectedArgSize = ArgI.Size;

            if (!ExpectedArgSize) ExpectedArgSize = 8;
        } else {
            ExpectedArgSize = FuncData.Params[i]->TypeInfo.Size;
        }

        if (ArgI.Size > 8) {
            emit_error("Function parameters must be at most 8 bytes large.");
            continue;
        }

        emit_move(code, Reg(ParamRegisters[i], ExpectedArgSize), ArgI);
    }

    if (IsVariadic) emit_move(code, Reg(REG_RAX, 1), Imm(FloatCount));

    emit(code, (asm_instruction){.Op = ASM_CALL, .Dst = LabelOperand(Name)});

    int ReturnSize = get_type_size(node->Call.FuncName->Ident.Sym->Function.ReturnType);

    // void
    if (ReturnSize == 0) return (operand){};

    assert(ReturnSize > 0);

    return Reg(REG_RAX, ReturnSize);
}

// Emit instructions and generate a resulting operand:
// Returns: either RAX, MEM, or IMM
operand emit_expression(ast_node *node, program_code *code) {
    operand Result = {};

    switch (node->Type) {
        case NODE_IDENT: {
            symbol *Sym = node->Ident.Sym;

            if (Sym->Section == SECTION_REG) {
                Result.Type         = OPERAND_REG;
                Result.Size         = Sym->TypeInfo.Size;
                Result.Reg.Register = ParamRegisters[Sym->ParamIndex];
            } else {
                Result.Type = OPERAND_MEM;

                if (Sym->TypeInfo.IsArray) {
                    Result.Size          = 8;
                    Result.Mem.IsAddress = true;
                } else {
                    Result.Size = Sym->TypeInfo.Size;
                }

                if (Sym->Section == SECTION_STACK) {
                    Result.Mem.Base         = REG_RBP;
                    Result.Mem.Displacement = ConstDisplacement(-Sym->StackOffset);
                }
            }
            break;
        }
        case NODE_CHAR_LIT: {
            Result.Type      = OPERAND_IMM;
            Result.Size      = 1;
            Result.Imm.Value = node->CharLit.Value;
            break;
        }
        case NODE_INT_LIT: {
            Result.Type      = OPERAND_IMM;
            Result.Size      = 0;
            Result.Imm.Value = node->IntegerLit.Value;
            break;
        }
        case NODE_STRING_LIT: {
            rodata_entry Entry;

            Entry.Type      = RODATA_STRING_LIT;
            Entry.StringLit = node->StringLit.Value;

            Result = get_string_lit_operand(code, push_rodata_entry(code, Entry));
            break;
        }

        case NODE_BINARY_OP: {
            return emit_binop(node, code);
        }

        case NODE_UNARY_OP: {
            return emit_unaryop(node, code);
        }

        case NODE_CALL: {
            return emit_call(node, code);
        }

        default: {
            break;
        }
    }

    if (Result.Type == OPERAND_REG) assert(Result.Reg.Register == REG_RAX);

    return Result;
}

operand scratch_register(int size) {
    if (NextFreeRegister >= ArraySize(ScratchRegisters)) {
        emit_error("Expression too complex, ran out of scratch registers.");
        assert(false);
        return (operand){};
    }

    NextFreeRegister++;

    if (NextFreeRegister > MaxScratchRegistersUsed) {
        MaxScratchRegistersUsed = NextFreeRegister;
    }

    return Reg(ScratchRegisters[NextFreeRegister - 1], size);
}

void free_scratch_register(void) {
    assert(NextFreeRegister > 0);
    NextFreeRegister--;
}

void emit_free_all_scratch_registers(program_code *code) {
    for (int i = MaxScratchRegistersUsed - 1; i >= 0; i--) {
        asm_instruction Instr = {.Op = ASM_POP, .Dst = Reg(ScratchRegisters[i], 8)};
        emit(code, Instr);
    }

    assert(NextFreeRegister == 0);
    MaxScratchRegistersUsed = 0;
}

void emit_move(program_code *code, operand Dst, operand Src) {
    if (Src.Size == 0) Src.Size = Dst.Size;

    // If we're moving from X to X, do nothing.
    if (Src.Type == OPERAND_REG && Dst.Type == OPERAND_REG && Src.Reg.Register == Dst.Reg.Register) {
        if (Src.Size >= Dst.Size) return;
        if (Dst.Size == 8) return;
    }

    if (Src.Type == OPERAND_IMM) {
        if (Dst.Type == OPERAND_REG && Src.Imm.Value == 0) {
            emit(code, (asm_instruction){.Op = ASM_XOR, .Dst = Dst, .Src = Dst});
        } else {
            emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Dst, .Src = Src});
        }
        return;
    }

    if (Src.Type == OPERAND_MEM && Src.Mem.IsAddress) {
        if (Dst.Type == OPERAND_REG) {
            emit_lea(code, Dst, Src);
        } else {
            operand Tmp = scratch_register(Src.Size);
            emit_lea(code, Tmp, Src);
            emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Dst, .Src = Tmp});
            free_scratch_register();
        }

        return;
    }

    if (Dst.Size < Src.Size) {
        Src.Size = Dst.Size;
    }

    bool NeedsExtend = Dst.Size > Src.Size && Dst.Size != 8;
    asm_opcode Op    = NeedsExtend ? ASM_MOVZX : ASM_MOV;

    if (Dst.Size == 8 && Src.Size == 4) Dst.Size = 4;

    if (Dst.Type == OPERAND_MEM && (NeedsExtend || Src.Type == OPERAND_MEM)) {
        operand Tmp = scratch_register(Dst.Size);
        emit(code, (asm_instruction){.Op = Op, .Dst = Tmp, .Src = Src});
        emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Dst, .Src = Tmp});
        free_scratch_register();
    } else {
        emit(code, (asm_instruction){.Op = Op, .Dst = Dst, .Src = Src});
    }
}

void emit_var_decl(program_code *code, ast_node *node) {
    for (int i = 0; i < node->VarDecl.ChildDeclsCount; i++) {
        emit_var_decl(code, node->VarDecl.ChildDecls[i]);
    }

    ast_node *Init = node->VarDecl.Init;

    if (!Init) return;

    ast_node *Ident = node->VarDecl.Name;
    symbol *Sym     = Ident->Ident.Sym;

    int Offset = Sym->StackOffset;

    // Variable is stored at [rbp - Offset]
    operand Mem = {
        .Type = OPERAND_MEM,
        .Size = Sym->TypeInfo.Size,
        .Mem  = {.Base = REG_RBP, .Index = REG_NONE, .Scale = 0, .Displacement = ConstDisplacement(-Offset)}
    };

    operand Value = emit_expression(Init, code);

    emit_move(code, Mem, Value);
}

void emit_statement(ast_node *node, program_code *code) {
    if (!node) return;

    switch (node->Type) {
        case NODE_PROGRAM: {
            for (int i = 0; i < node->Program.FunctionCount; i++) {
                emit_statement(node->Program.Functions[i], code);
            }
            break;
        }
        case NODE_INCLUDE: {
            break;
        }
        case NODE_FUNC_DEF: {
            if (!node->FuncDef.Body) return;  // Forward declaration

            ast_node *Name = node->FuncDef.Name;

            code->CurrentFunction           = Name->Ident.Name;
            code->CurrentFunctionReturnSize = get_type_size(node->FuncDef.ReturnType->DataType.Type);

            for (int i = 0; i < node->FuncDef.ParamCount; i++) {
                int Size = get_type_size(node->FuncDef.Params[i]->VarDecl.Type->DataType.Type);

                if (Size == -1) {
                    symbol *Sym = node->FuncDef.Params[i]->VarDecl.Type->DataType.Name->Ident.Sym;
                    Size        = Sym->TypeInfo.Size;
                }

                if (Size > 8) {
                    emit_error("Cannot pass types larger than 8 bytes as a parameter.");
                }
            }

            size_t local_size = Name->Ident.Sym->TypeInfo.Size;
            size_t frame_size = local_size + total_param_bytes(Name->Ident.Sym);

            string_equals(Name->Ident.Name, CSTR("main"));

            emit_function_label(code, Name->Ident.Name);

            asm_instruction *SubRspInstr;
            asm_instruction *Scratch = emit_function_prologue(code, &SubRspInstr);

            emit_spill_params(code, Name->Ident.Sym, local_size);

            emit_statement(node->FuncDef.Body, code);

            // Backfill the scratch regsiters
            for (int i = 0; i < MaxScratchRegistersUsed; i++) {
                Scratch[i] = (asm_instruction){.Op = ASM_PUSH, .Dst = Reg(ScratchRegisters[i], 8)};
            }

            // Backfill the local size, ensuring % 16 == 8.
            int LogicalFrameSize = frame_size + NextFreeRegister * 8;

            if (LogicalFrameSize % 16 > 0) frame_size += 16 - LogicalFrameSize % 16;

            SubRspInstr->Src.Imm.Value = frame_size;

            emit_function_epilogue(code);

            break;
        }
        case NODE_BLOCK: {
            for (int i = 0; i < node->Block.StatementCount; i++) {
                emit_statement(node->Block.Statements[i], code);
            }
            break;
        }
        case NODE_BINARY_OP: {
            ast_node *Left  = node->BinaryOp.Left;
            ast_node *Right = node->BinaryOp.Right;

            switch (node->BinaryOp.Operation) {
                case TOKEN_EQUALS: {
                    operand Src = emit_expression(Right, code);

                    bool UsedScratch = false;

                    if (is_node_dereference(Right)) {
                        assert(Src.Type == OPERAND_MEM);
                        assert(Src.Mem.Base == REG_RAX);
                        // Move the pointer result out of RAX, since
                        // it's going to be clobbered by the next emit_expression
                        operand Temp = scratch_register(8);  // (pointer size)
                        emit_move(code, Temp, Reg(Src.Mem.Base, 8));
                        Src.Mem.Base = Temp.Reg.Register;
                        UsedScratch  = true;
                    }

                    operand Dst = emit_expression(Left, code);

                    emit_move(code, Dst, Src);

                    if (UsedScratch) {
                        free_scratch_register();
                    }
                    break;
                }
                default: {
                    emit_error("Haven't implemented parsing for binary operation of type %s yet, sorry!",
                               token_name(node->BinaryOp.Operation));
                    break;
                }
            }
            break;
        }
        case NODE_UNARY_OP:
        case NODE_CALL:     {
            emit_expression(node, code);
            break;
        }
        case NODE_IF: {
            operand Result = emit_expression(node->If.Condition, code);

            string L_Else = new_label(code);

            emit_test(code, Result, Result);

            emit_je(code, L_Else);

            emit_statement(node->If.ThenBlock, code);

            if (node->If.ElseBlock) {
                string L_End = new_label(code);
                emit_jump(code, L_End);
                emit_label(code, L_Else);
                emit_statement(node->If.ElseBlock, code);
                emit_label(code, L_End);
            } else {
                emit_label(code, L_Else);
            }
            break;
        }
        case NODE_WHILE: {
            string Start = emit_next_label(code);
            string End   = new_label(code);

            push_loop(code, End, Start);

            operand Condition = emit_expression(node->While.Condition, code);

            emit_test(code, Condition, Condition);
            emit_je(code, End);

            emit_statement(node->While.Body, code);

            emit_jump(code, Start);

            emit_label(code, End);

            pop_loop(code);
            break;
        }
        case NODE_FOR: {
            string Start    = new_label(code);
            string End      = new_label(code);
            string Continue = new_label(code);

            push_loop(code, End, Continue);

            emit_statement(node->For.Init, code);

            emit_label(code, Start);

            operand Condition = emit_expression(node->For.Condition, code);

            emit_test(code, Condition, Condition);
            emit_je(code, End);

            emit_statement(node->For.Body, code);
            emit_label(code, Continue);
            emit_statement(node->For.Advance, code);
            emit_jump(code, Start);
            emit_label(code, End);

            pop_loop(code);
            break;
        }
        case NODE_RETURN: {
            ast_node *Val   = node->Return.Value;
            operand Operand = emit_expression(Val, code);

            // TODO: What if we return a struct (or float)?
            emit_move(code, Reg(REG_RAX, code->CurrentFunctionReturnSize), Operand);

            emit_jump(code, function_end_label(code));
            break;
        }
        case NODE_BREAK: {
            emit_jump(code, code->Loops[code->LoopCount - 1].BreakLabel);
            break;
        }
        case NODE_CONTINUE: {
            emit_jump(code, code->Loops[code->LoopCount - 1].ContinueLabel);
            break;
        }
        case NODE_VAR_DECL: {
            emit_var_decl(code, node);
            break;
        }
        case NODE_STRUCT: break;
        default:
            printf("Haven't implemented codegen for node type ");
            print_node_type(node);
            printf("\n");
            break;
    }
}

char *register_name(register_id Reg, int Size) {
    int Index;

    switch (Size) {
        case 1: Index = 0; break;
        case 2: Index = 1; break;
        case 4: Index = 2; break;
        case 8: Index = 3; break;
        default:
            printf("Size is incorrect %d\n", Size);
            assert(false);
            break;
    }

    static const char *RegNames[][4] = {
        [REG_RAX] = {"al",  "ax",  "eax", "rax"},
        [REG_RBX] = {"bl",  "bx",  "ebx", "rbx"},
        [REG_RCX] = {"cl",  "cx",  "ecx", "rcx"},
        [REG_RDX] = {"dl",  "dx",  "edx", "rdx"},
        [REG_RSI] = {"sil", "si",  "esi", "rsi"},
        [REG_RDI] = {"dil", "di",  "edi", "rdi"},
        [REG_RBP] = {"bpl", "bp",  "ebp", "rbp"},
        [REG_RSP] = {"spl", "sp",  "esp", "rsp"},
        [REG_RIP] = {"err", "err", "err", "rip"},
    };

    switch (Reg) {
        case REG_NONE: return "(none)";

        case REG_RAX:
        case REG_RBX:
        case REG_RCX:
        case REG_RDX:
        case REG_RSI:
        case REG_RDI:
        case REG_RBP:
        case REG_RIP:
        case REG_RSP: return (char *)RegNames[Reg][Index];

        case REG_R8:
        case REG_R9:
        case REG_R10:
        case REG_R11:
        case REG_R12:
        case REG_R13:
        case REG_R14:
        case REG_R15: {
            int RegNum                   = (int)Reg - REG_R8 + 8;
            static const char Suffixes[] = "bwd";
            char Suffix                  = Suffixes[Index];

            static char Result[32] = {};

            if (Suffix != '\0') {
                sprintf(Result, "r%d%c", RegNum, Suffix);
            } else {
                sprintf(Result, "r%d", RegNum);
            }

            return Result;
        }

        default: break;
    }

    return "(null)";
}

char *size_directive(int size) {
    switch (size) {
        case 0: return "";  // Used for LEA
        case 1: return "BYTE PTR";
        case 2: return "WORD PTR";
        case 4: return "DWORD PTR";
        case 8: return "QWORD PTR";
    }

    return "(invalid size)";
}

void print_mem(FILE *out, operand op) {
    char *SizeDirective = size_directive(op.Size);
    fprintf(out, "%s%s[%s", SizeDirective, SizeDirective[0] ? " " : "", register_name(op.Mem.Base, 8));

    int RegSize = op.Size > 0 ? op.Size : 8;
    if (op.Mem.Index) fprintf(out, " + %s*%d", register_name(op.Mem.Index, RegSize), op.Mem.Scale);

    displacement Disp = op.Mem.Displacement;

    if (Disp.Type == DISPLACEMENT_CONSTANT) {
        int Off = Disp.Value;

        if (Off >= 0) {
            fprintf(out, " + ");
        } else {
            fprintf(out, " - ");
            Off = -Off;
        }

        fprintf(out, "%d]", Off);
    } else if (Disp.Type == DISPLACEMENT_LABEL) {
        fprintf(out, " + .%.*s]", (int)Disp.Label.Length, Disp.Label.Data);
    } else {
        fprintf(out, "]");
    }
}

void print_operand(FILE *out, operand op) {
    switch (op.Type) {
        case OPERAND_NONE:  fprintf(out, "none"); break;
        case OPERAND_REG:   fprintf(out, "%s", register_name(op.Reg.Register, op.Size)); break;
        case OPERAND_IMM:   fprintf(out, "%lld", (long long)op.Imm.Value); break;
        case OPERAND_MEM:   print_mem(out, op); break;
        case OPERAND_LABEL: fprintf(out, "%.*s", (int)op.Label.Name.Length, op.Label.Name.Data); break;
        default:            fprintf(out, "operand(?)"); break;
    }
}

char *instruction_name(asm_opcode Opcode) {
    switch (Opcode) {
        case ASM_INVALID: return "";
        case ASM_LABEL:   return "label";
        case ASM_MOV:     return "mov";
        case ASM_LEA:     return "lea";
        case ASM_AND:     return "and";
        case ASM_OR:      return "or";
        case ASM_XOR:     return "xor";
        case ASM_NOT:     return "not";
        case ASM_SHL:     return "shl";
        case ASM_SHR:     return "shr";
        case ASM_SAR:     return "sar";
        case ASM_CDQ:     return "cdq";
        case ASM_MOVSX:   return "movsx";
        case ASM_NOP:     return "nop";
        case ASM_PUSH:    return "push";
        case ASM_POP:     return "pop";
        case ASM_ADD:     return "add";
        case ASM_SUB:     return "sub";
        case ASM_IMUL:    return "imul";
        case ASM_IDIV:    return "idiv";
        case ASM_NEG:     return "neg";
        case ASM_CMP:     return "cmp";
        case ASM_TEST:    return "test";
        case ASM_SETE:    return "sete";
        case ASM_SETNE:   return "setne";
        case ASM_SETL:    return "setl";
        case ASM_SETLE:   return "setle";
        case ASM_SETG:    return "setg";
        case ASM_SETGE:   return "setge";
        case ASM_MOVZX:   return "movzx";
        case ASM_JMP:     return "jmp";
        case ASM_JE:      return "je";
        case ASM_JNE:     return "jne";
        case ASM_JL:      return "jl";
        case ASM_JLE:     return "jle";
        case ASM_JG:      return "jg";
        case ASM_JGE:     return "jge";
        case ASM_CALL:    return "call";
        case ASM_RET:     return "ret";
        case ASM_SYSCALL: return "syscall";
    }

    return "[Invalid Instruction]";
}
int number_operands(asm_opcode Op) {
    switch (Op) {
        case ASM_INVALID: return 0;
        case ASM_LABEL:   return 0;
        case ASM_MOV:     return 2;
        case ASM_LEA:     return 2;
        case ASM_AND:     return 2;
        case ASM_OR:      return 2;
        case ASM_XOR:     return 2;
        case ASM_NOT:     return 1;
        case ASM_SHL:     return 2;
        case ASM_SHR:     return 2;
        case ASM_SAR:     return 2;
        case ASM_CDQ:     return 0;
        case ASM_MOVSX:   return 2;
        case ASM_NOP:     return 0;
        case ASM_PUSH:    return 1;
        case ASM_POP:     return 1;
        case ASM_ADD:     return 2;
        case ASM_SUB:     return 2;
        case ASM_IMUL:    return 2;
        case ASM_IDIV:    return 1;
        case ASM_NEG:     return 1;
        case ASM_CMP:     return 2;
        case ASM_TEST:    return 2;
        case ASM_SETE:    return 1;
        case ASM_SETNE:   return 1;
        case ASM_SETL:    return 1;
        case ASM_SETLE:   return 1;
        case ASM_SETG:    return 1;
        case ASM_SETGE:   return 1;
        case ASM_MOVZX:   return 2;
        case ASM_JMP:     return 1;
        case ASM_JE:      return 1;
        case ASM_JNE:     return 1;
        case ASM_JL:      return 1;
        case ASM_JLE:     return 1;
        case ASM_JG:      return 1;
        case ASM_JGE:     return 1;
        case ASM_CALL:    return 1;
        case ASM_RET:     return 0;
        case ASM_SYSCALL: return 0;
    }
}

void print_instruction(FILE *out, asm_instruction *in) {
    switch (in->Op) {
        case ASM_LABEL: {
            string_print_to(out, in->Dst.Label.Name);
            fprintf(out, ":");
            break;
        }
        default: {
            fprintf(out, "%s ", instruction_name(in->Op));

            int NumberOperands = number_operands(in->Op);

            if (NumberOperands == 0) break;

            print_operand(out, in->Dst);

            if (NumberOperands == 1) break;

            fprintf(out, ", ");
            print_operand(out, in->Src);
            break;
        }
    }

    fputc('\n', out);
}

void dbg_operand(operand op) {
    print_operand(stdout, op);
    puts("");
    fflush(stdout);
}
void dbg_instr(asm_instruction *in) {
    print_instruction(stdout, in);
    puts("");
    fflush(stdout);
}

program_code gen_program_code(FILE *out, memory_arena *arena, ast_node *ast) {
    program_code Code = {};

    Code.InstructionArena = make_arena();
    Code.GeneralArena     = arena;

    Code.RodataEntries = arena_push(arena, MAX_STRING_LIT * sizeof(rodata_entry));
    Code.Loops         = arena_push(arena, MAX_DEPTH * sizeof(loop));

    emit_statement(ast, &Code);

    asm_instruction *Instructions = (asm_instruction *)Code.InstructionArena.Data;

    fprintf(out, ".intel_syntax noprefix\n\n");

    if (Code.RodataEntryCount > 0) {
        fprintf(out, ".section .rodata\n");

        for (int i = 0; i < Code.RodataEntryCount; i++) {
            rodata_entry *Entry = Code.RodataEntries + i;

            if (Entry->Type == RODATA_STRING_LIT) {
                fprintf(out,
                        ".%.*s: .asciz \"%.*s\"\n",
                        (int)Entry->Label.Length,
                        Entry->Label.Data,
                        (int)Entry->StringLit.Length,
                        Entry->StringLit.Data);
            }
        }

        fprintf(out, "\n");
    }

    fprintf(out, ".section .text\n.global main\n\n");

    memory_arena AsmArena = make_arena();

    for (int i = 0; i < Code.InstructionCount; i++) {
        asm_instruction *Instr = Instructions + i;

        if (Instr->Op == 0) continue;
        if (Instr->Op != ASM_LABEL) fprintf(out, "  ");

        print_instruction(out, Instr);
    }

    fputs("\n.section .note.GNU-stack,\"\",@progbits\n", out);

    free_arena(&AsmArena);
    free_arena(&Code.InstructionArena);

    return Code;
}

void free_program_code(program_code *program) { free_arena(&program->InstructionArena); }
