#include "gen.h"

#include <assert.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>

#include "util/util.h"

#define Imm(value) \
    (operand) { .Type = OPERAND_IMM, .Imm.Value = value }

#define Reg(reg, size) \
    (operand) { .Type = OPERAND_REG, .Size = size, .Reg.Register = reg }
#define Rbp (Reg(REG_RBP, SIZE_64))
#define Rsp (Reg(REG_RSP, SIZE_64))

#define ConstDisplacement(disp) \
    (displacement) { .Type = DISPLACEMENT_CONSTANT, .Value = (disp) }
#define LabelDisplacement(label) \
    (displacement) { .Type = DISPLACEMENT_LABEL, .Label = (label) }

constexpr register_id ParamRegisters[]   = {REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9};
constexpr register_id ScratchRegisters[] = {REG_RBX, REG_R12, REG_R13, REG_R14, REG_R15};

int NextFreeRegister = 0;

void emit_error(const char *format, ...) {
    va_list Args;
    va_start(Args, format);

    printf("[Codegen Error] ");

    vprintf(format, Args);
    printf("\n");

    va_end(Args);
}

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

asm_instruction *emit_function_prologue(program_code *code, size_t frame_size) {
    // TODO: If the function is a leaf function, account for 128 byte redzone?
    size_t AlignedSize = 16 * (frame_size / 16 + (frame_size % 16 > 0));

    asm_instruction Instructions[] = {
        {.Op = ASM_PUSH, .Dst = Rbp},
        {.Op = ASM_MOV, .Dst = Rbp, .Src = Rsp},
        {.Op = ASM_SUB, .Dst = Rsp, .Src = Imm(AlignedSize)},
    };

    int Count = ArraySize(Instructions);

    if (AlignedSize == 0) Count--;

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
        {.Op = ASM_MOV, .Dst = Reg(REG_RDI, SIZE_64), .Src = Reg(REG_RAX, SIZE_64)},
        {.Op = ASM_MOV, .Dst = Reg(REG_RAX, SIZE_64), .Src = Imm(60)},
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
    emit_count(code, Instructions, ArraySize(Instructions));
}

void emit_for(program_code *code) {}

operand LabelOperand(string Label) { return (operand){.Type = OPERAND_LABEL, .Label.Name = Label}; }

void emit_jump(program_code *code, string Label) {
    emit(code, (asm_instruction){.Op = ASM_JMP, .Dst = LabelOperand(Label)});
}

void emit_cmp(program_code *code, operand Left, operand Right) {
    if (Left.Size == SIZE_NONE)
        Left.Size = Right.Size;
    else if (Right.Size == SIZE_NONE)
        Right.Size = Left.Size;

    if (Left.Size < Right.Size) {
        Right.Size = Left.Size;
    }

    if (Left.Type == OPERAND_MEM && Right.Type == OPERAND_MEM) {
        operand Tmp = scratch_register(Left.Size);
        emit_move(code, Tmp, Left);
        emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Tmp, .Src = Right});
    } else {
        emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Left, .Src = Right});
    }
}

void emit_test(program_code *code, operand A, operand B) {
    operand Tmp;

    if (A.Type != OPERAND_REG) {
        Tmp = scratch_register(A.Size);
        emit_move(code, Tmp, A);
        A = Tmp;
        B = Tmp;
    }

    emit(code, (asm_instruction){.Op = ASM_TEST, .Dst = A, .Src = B});
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

    emit(code, (asm_instruction){.Op = SetOpcode, .Dst = Reg(REG_RAX, SIZE_8)});
    emit(code, (asm_instruction){.Op = ASM_MOVZX, .Dst = Reg(REG_RAX, SIZE_32), .Src = Reg(REG_RAX, SIZE_8)});

    return Reg(REG_RAX, SIZE_32);
}

operand get_string_lit_operand(program_code *code, string Label) {
    return (operand){
        .Type = OPERAND_MEM, .Mem = {.IsAddress = true, .Base = REG_RIP, .Displacement = LabelDisplacement(Label)}
    };
}

void emit_lea(program_code *code, operand DstReg, operand SrcMem) {
    assert(DstReg.Type == OPERAND_REG);
    assert(SrcMem.Type == OPERAND_MEM);

    emit(code, (asm_instruction){.Op = ASM_LEA, .Dst = DstReg, .Src = SrcMem});
}

operand emit_math(program_code *code, token_type Op, operand Left, operand Right) {
    int Size = Max(Left.Size, Right.Size);

    if (Size == 0) Size = SIZE_32;

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

    if (MathOp == ASM_IDIV) {
        emit_move(code, Reg(REG_RAX, Size), Left);

        // Why did we do this again...?
        emit(code, (asm_instruction){.Op = ASM_CDQ});

        if (Right.Type == OPERAND_IMM) {
            Tmp = scratch_register(Size);
            emit_move(code, Tmp, Right);
            Right = Tmp;
        }

        emit(code, (asm_instruction){.Op = ASM_IDIV, .Dst = Right});
    } else {
        if (Right.Type == OPERAND_REG && Right.Reg.Register == REG_RAX) {
            Tmp = scratch_register(Size);
            emit_move(code, Tmp, Right);
            Right = Tmp;
        }
        emit_move(code, Reg(REG_RAX, Size), Left);
        emit(code, (asm_instruction){.Op = MathOp, .Dst = Reg(REG_RAX, Size), .Src = Right});
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

    if (Left.Type == OPERAND_REG && Left.Reg.Register == REG_RAX && expr_may_clobber_rax(node->BinaryOp.Right)) {
        SafeLeft = scratch_register(Left.Size);
        emit_move(code, SafeLeft, Left);
        Left = SafeLeft;
    }

    operand Right = emit_expression(node->BinaryOp.Right, code);

    operand Result;

    switch (Op) {
        case TOKEN_EQUALS_EQUALS:
        case TOKEN_BANG_EQUALS:   {
            emit_cmp(code, Left, Right);

            asm_opcode Set = Op == TOKEN_EQUALS_EQUALS ? ASM_SETE : ASM_SETNE;

            emit(code, (asm_instruction){.Op = Set, .Dst = Reg(REG_RAX, SIZE_8)});

            emit(code, (asm_instruction){.Op = ASM_MOVZX, .Dst = Reg(REG_RAX, SIZE_32), .Src = Reg(REG_RAX, SIZE_8)});

            Result = Reg(REG_RAX, SIZE_32);
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
                Result = emit_pointer_math_op(code, node);
                break;
            }
        case TOKEN_STAR:
        case TOKEN_DIVIDE: {
            Result = emit_math(code, node->BinaryOp.Operation, Left, Right);
            break;
        }

        default: {
            emit_error("Didn't implement this binary operation %s\n", token_name(Op));
            Result = (operand){};
            break;
        }
    }

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

operand emit_inc_dec(program_code *Code, ast_node *Node, bool Increment, bool Prefix) {
    operand Var = emit_expression(Node, Code);

    if (Var.Type == OPERAND_IMM) {
        emit_error("Can't ++ or -- an immediate value.");
        return (operand){};
    }

    operand Temp = scratch_register(Var.Size);

    if (!Prefix) {
        emit_move(Code, Temp, Var);
    }

    operand Right = Imm(1);

    operand Result = emit_math(Code, Increment ? TOKEN_PLUS : TOKEN_MINUS, Var, Right);

    emit_move(Code, Var, Result);

    return Prefix ? Var : Temp;
}

// Get type info of a pointer from pointer arithmetic binary operation
type_info get_type_info_from_operand(ast_node *Node) {
    type_info Result = {};

    switch (Node->Type) {
        case NODE_IDENT:     Result = Node->Ident.Sym->TypeInfo; break;
        case NODE_BINARY_OP: {
            if (Node->BinaryOp.Operation != TOKEN_PLUS) emit_error("Pointer math can only be done with addition.");

            type_info A = get_type_info_from_operand(Node->BinaryOp.Left);
            type_info B = get_type_info_from_operand(Node->BinaryOp.Right);

            if (A.IndirectionDepth > 0 && B.IndirectionDepth > 0) {
                emit_error("Pointer math can only be done with one pointer operand.");
            } else if (A.IndirectionDepth > 0) {
                Result = A;
            } else {
                Result = B;
            }
        }
        default: break;
    }

    return Result;
}

bool is_pointer_math_op(program_code *code, ast_node *Node) {
    if (Node->Type != NODE_BINARY_OP) return false;

    type_info A = get_type_info_from_operand(Node->BinaryOp.Left);
    type_info B = get_type_info_from_operand(Node->BinaryOp.Right);

    if (A.IndirectionDepth > 0 && B.IndirectionDepth > 0) return false;

    if (A.IndirectionDepth > 0 || B.IndirectionDepth > 0) return true;

    return false;
}

// Returns pointer into rax
operand emit_pointer_math_op(program_code *code, ast_node *Operand) {
    type_info TypeInfo = get_type_info_from_operand(Operand);

    switch (Operand->Type) {
        case NODE_IDENT: {
            operand Mem = emit_expression(Operand, code);
            emit_move(code, Reg(REG_RAX, SIZE_64), Mem);
            return Reg(REG_RAX, SIZE_64);
        }
        case NODE_BINARY_OP: {
            token_type Operation = Operand->BinaryOp.Operation;

            switch (Operation) {
                case TOKEN_PLUS:
                case TOKEN_MINUS: {
                    // Right now I've only hardcoded ptr + n syntax. But ptr + n1 + n2 + ... should be valid too, no?
                    // Not really a high priority for me though, so I'm leaving it so.

                    operand Left  = emit_expression(Operand->BinaryOp.Left, code);
                    operand Right = emit_expression(Operand->BinaryOp.Right, code);

                    type_info TypeInfoLeft = get_type_info_from_operand(Operand->BinaryOp.Left);

                    operand Ptr = TypeInfoLeft.IndirectionDepth > 0 ? Left : Right;
                    operand Off = TypeInfoLeft.IndirectionDepth > 0 ? Right : Left;

                    Off.Imm.Value *= TypeInfo.PointingTo->Size;

                    // Move result into rax
                    emit_move(code, Reg(REG_RAX, SIZE_64), Ptr);

                    asm_opcode Opcode = Operation == TOKEN_PLUS ? ASM_ADD : ASM_SUB;

                    operand Src = Off;

                    if (Off.Size != SIZE_64) {
                        // move (sign extended) into temp register
                        operand Tmp = scratch_register(SIZE_64);
                        emit(code, (asm_instruction){.Op = ASM_MOVSX, .Dst = Tmp, .Src = Off});
                        Src = Tmp;
                    }

                    emit(code, (asm_instruction){.Op = Opcode, .Dst = Reg(REG_RAX, SIZE_64), .Src = Src});

                    return Reg(REG_RAX, SIZE_64);
                    break;
                }
                default: {
                    // TODO: Error message
                    assert(false);
                    break;
                }
            }
            break;
        }
        default: {
            // TODO: Error message
            assert(false);
            break;
        }
    }

    assert(false);

    return (operand){};
}

operand emit_dereference(program_code *code, ast_node *OperandNode) {
    assert(OperandNode->Type == NODE_UNARY_OP);

    // Extract type info to get size
    type_info TypeInfo = get_type_info_from_operand(OperandNode->UnaryOp.Operand);

    emit_pointer_math_op(code, OperandNode->UnaryOp.Operand);

    if (TypeInfo.Size <= 8) {
        operand Dst = {};

        Dst.Type          = OPERAND_MEM;
        Dst.Size          = TypeInfo.PointingTo->Size;
        Dst.Mem.IsAddress = true;
        Dst.Mem.Base      = REG_RAX;

        return Dst;
    } else {
        // TODO: Emit memcpy?
        assert(false);
    }

    return Reg(REG_RAX, SIZE_64);
}

operand emit_unaryop(ast_node *node, program_code *code) {
    token_type Operation = node->UnaryOp.Operation;

    if (Operation == TOKEN_STAR) {
        return emit_dereference(code, node);
    }

    operand Operand = emit_expression(node->UnaryOp.Operand, code);

    bool Prefix = node->UnaryOp.First;

    switch (Operation) {
        case TOKEN_MINUS: return emit_negate(code, node, Operand);
        case TOKEN_INC:   return emit_inc_dec(code, node->UnaryOp.Operand, true, Prefix);
        case TOKEN_DEC:   return emit_inc_dec(code, node->UnaryOp.Operand, false, Prefix);
        default:          assert(false); break;
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

    for (int i = node->Call.ArgCount - 1; i >= 0; i--) {
        operand ArgI        = emit_expression(node->Call.Args[i], code);
        int ExpectedArgSize = node->Call.FuncName->Ident.Sym->Function.Params[i]->TypeInfo.Size;

        if (ArgI.Size > 8) {
            emit_error("Function parameters must be at most 8 bytes large.");
            continue;
        }

        emit_move(code, Reg(ParamRegisters[i], ExpectedArgSize), ArgI);
    }

    emit(code, (asm_instruction){.Op = ASM_CALL, .Dst = LabelOperand(Name)});

    int ReturnSize = get_type_size(node->Call.FuncName->Ident.Sym->Function.ReturnType);

    // void
    if (ReturnSize == 0) return (operand){};

    assert(ReturnSize > 0);

    return Reg(REG_RAX, ReturnSize);
}

// Emit instructions and generate a resulting operand
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
                Result.Size = Sym->TypeInfo.Size;

                if (Sym->Section == SECTION_STACK) {
                    Result.Mem.Base         = REG_RBP;
                    Result.Mem.Displacement = ConstDisplacement(-Sym->StackOffset);
                }
            }
            break;
        }
        case NODE_CHAR_LIT: {
            Result.Type      = OPERAND_IMM;
            Result.Size      = SIZE_8;
            Result.Imm.Value = node->CharLit.Value;
            break;
        }
        case NODE_INT_LIT: {
            Result.Type      = OPERAND_IMM;
            Result.Size      = SIZE_NONE;
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

    return Result;
}

operand scratch_register(operand_size size) {
    if (NextFreeRegister >= ArraySize(ScratchRegisters)) {
        emit_error("Expression too complex, ran out of scratch registers.");
        return (operand){};
    }

    return Reg(ScratchRegisters[NextFreeRegister++], size);
}

void emit_free_all_scratch_registers(program_code *code) {
    for (int i = NextFreeRegister - 1; i >= 0; i--) {
        asm_instruction Instr = {.Op = ASM_POP, .Dst = Reg(ScratchRegisters[i], SIZE_64)};
        emit(code, Instr);
    }

    NextFreeRegister = 0;
}

void emit_move(program_code *code, operand Dst, operand Src) {
    if (Src.Size == SIZE_NONE) Src.Size = Dst.Size;

    if (Src.Type == OPERAND_REG && Dst.Type == OPERAND_REG && Src.Reg.Register == Dst.Reg.Register) {
        if (Src.Size >= Dst.Size) return;
        if (Dst.Size == SIZE_64) return;
    }

    if (Src.Type == OPERAND_IMM) {
        emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Dst, .Src = Src});
        return;
    }

    if (Src.Type == OPERAND_MEM && Src.Mem.IsAddress) {
        if (Dst.Type == OPERAND_REG) {
            emit_lea(code, Dst, Src);
        } else {
            operand Tmp = scratch_register(Src.Size);
            emit_lea(code, Tmp, Src);
            emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Dst, .Src = Tmp});
        }

        return;
    }

    if (Dst.Size < Src.Size) {
        Src.Size = Dst.Size;
    }

    bool NeedsExtend = Dst.Size > Src.Size && Dst.Size != SIZE_64;
    asm_opcode Op    = NeedsExtend ? ASM_MOVZX : ASM_MOV;

    if (Dst.Type == OPERAND_MEM && (NeedsExtend || Src.Type == OPERAND_MEM)) {
        operand Tmp = scratch_register(Dst.Size);
        emit(code, (asm_instruction){.Op = Op, .Dst = Tmp, .Src = Src});
        emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Dst, .Src = Tmp});
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
        case NODE_PROGRAM:
            for (int i = 0; i < node->Program.FunctionCount; i++) {
                emit_statement(node->Program.Functions[i], code);
            }
            break;
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

            bool is_main = string_equals(Name->Ident.Name, CSTR("main"));

            emit_function_label(code, Name->Ident.Name);

            asm_instruction *Scratch = emit_function_prologue(code, frame_size);

            emit_spill_params(code, Name->Ident.Sym, local_size);

            emit_statement(node->FuncDef.Body, code);

            for (int i = 0; i < NextFreeRegister; i++) {
                Scratch[i] = (asm_instruction){.Op = ASM_PUSH, .Dst = Reg(ScratchRegisters[i], SIZE_64)};
            }

            emit_free_all_scratch_registers(code);

            if (is_main) {
                emit_program_epilogue(code);
            } else {
                emit_function_epilogue(code);
            }

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
                    operand Dst = emit_expression(Left, code);

                    emit_move(code, Dst, Src);
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

char *register_name(memory_arena *Arena, register_id Reg, operand_size Size) {
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

            // Allocate buffer from the arena (e.g., "r15d\0" fits inside 5-6 bytes)
            char *Result = (char *)arena_push(Arena, 6);
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

char *size_directive(operand_size size) {
    switch (size) {
        case SIZE_NONE: return "(invalid size)";
        case SIZE_8:    return "BYTE PTR";
        case SIZE_16:   return "WORD PTR";
        case SIZE_32:   return "DWORD PTR";
        case SIZE_64:   return "QWORD PTR";
    }

    return "(invalid size)";
}

void print_mem(memory_arena *Arena, FILE *out, operand op) {
    fprintf(out, "%s [%s", size_directive(op.Size), register_name(Arena, op.Mem.Base, SIZE_64));
    if (op.Mem.Index) fprintf(out, " + %s*%d", register_name(Arena, op.Mem.Index, op.Size), op.Mem.Scale);

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

void print_operand(memory_arena *Arena, FILE *out, operand op) {
    switch (op.Type) {
        case OPERAND_NONE:  fprintf(out, "none"); break;
        case OPERAND_REG:   fprintf(out, "%s", register_name(Arena, op.Reg.Register, op.Size)); break;
        case OPERAND_IMM:   fprintf(out, "%lld", (long long)op.Imm.Value); break;
        case OPERAND_MEM:   print_mem(Arena, out, op); break;
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

void print_instruction(memory_arena *Arena, FILE *out, asm_instruction *in) {
    switch (in->Op) {
        case ASM_LABEL: {
            string_print_to(out, in->Dst.Label.Name);
            fprintf(out, ":");
            break;
        }
        default: {
            fprintf(out, "%s ", instruction_name(in->Op));

            if (in->Op == ASM_NOP || in->Op == ASM_SYSCALL || in->Op == ASM_RET || in->Op == ASM_CDQ) break;

            print_operand(Arena, out, in->Dst);

            if (in->Op == ASM_CALL || in->Op == ASM_IDIV || in->Op == ASM_SETL || in->Op == ASM_SETG ||
                in->Op == ASM_SETLE || in->Op == ASM_SETGE || in->Op == ASM_JE || in->Op == ASM_JMP ||
                in->Op == ASM_PUSH || in->Op == ASM_POP || in->Op == ASM_SETNE || in->Op == ASM_SETE)
                break;

            fprintf(out, ", ");
            print_operand(Arena, out, in->Src);
            break;
        }
    }

    fputc('\n', out);
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

        print_instruction(&AsmArena, out, Instr);
    }

    fputs("\n.section .note.GNU-stack,\"\",@progbits\n", out);

    free_arena(&AsmArena);
    free_arena(&Code.InstructionArena);

    return Code;
}

void free_program_code(program_code *program) { free_arena(&program->InstructionArena); }
