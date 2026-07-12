#include "gen.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "util/util.h"

#define Imm(value) \
    (operand) { .Type = OPERAND_IMM, .Imm.Value = value }

constexpr operand Al   = (operand){.Type = OPERAND_REG, .Size = SIZE_8, .Reg.Register = REG_AL};
constexpr operand Ax   = (operand){.Type = OPERAND_REG, .Size = SIZE_16, .Reg.Register = REG_AX};
constexpr operand Eax  = (operand){.Type = OPERAND_REG, .Size = SIZE_32, .Reg.Register = REG_EAX};
constexpr operand Rax  = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_RAX};
constexpr operand Rbp  = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_RBP};
constexpr operand Rsp  = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_RSP};
constexpr operand R11  = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_R11};
constexpr operand R11d = (operand){.Type = OPERAND_REG, .Size = SIZE_32, .Reg.Register = REG_R11d};

void gen_error(const char *format, ...) {
    va_list Args;
    va_start(Args, format);

    printf("[Codegen Error] ");

    vprintf(format, Args);
    printf("\n");

    va_end(Args);
}

void emit_count(program_code *code, asm_instruction *instructions, int count) {
    asm_instruction *Ptr = arena_push(&code->InstructionArena, count * sizeof(asm_instruction));

    if (Ptr) {
        memcpy(Ptr, instructions, count * sizeof(asm_instruction));
        code->InstructionCount += count;
    }
}

void emit(program_code *code, asm_instruction instruction) { emit_count(code, &instruction, 1); }

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

void emit_function_prologue(program_code *code, size_t local_size) {
    size_t AlignedSize = 16 * (local_size / 16 + (local_size % 16 > 0));

    asm_instruction Instructions[] = {
        {.Op = ASM_PUSH, .Dst = Rbp},
        {.Op = ASM_MOV, .Dst = Rbp, .Src = Rsp},
        {.Op = ASM_SUB, .Dst = Rsp, .Src = Imm(AlignedSize)},
    };

    emit_count(code, Instructions, ArraySize(Instructions));
}

string function_end_label(program_code *code) {
    string FunctionName = code->CurrentFunction;
    string Name         = string_make(code->GeneralArena, FunctionName.Length + 6);
    Name.Length         = sprintf(Name.Data, "Lend_%.*s", (int)FunctionName.Length, FunctionName.Data);
    return Name;
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

operand LabelOperand(string Label) { return (operand){.Type = OPERAND_LABEL, .Label.Name = Label}; }

void emit_jump(program_code *code, string Label) {
    emit(code, (asm_instruction){.Op = ASM_JMP, .Dst = LabelOperand(Label)});
}

void emit_cmp(program_code *code, operand Left, operand Right) {
    if (Left.Type == OPERAND_MEM && Right.Type == OPERAND_MEM) {
        operand Tmp = scratch_register(Right.Size);
        emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Tmp, .Src = Left});
        emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Tmp, .Src = Right});
    } else {
        emit(code, (asm_instruction){.Op = ASM_CMP, .Dst = Left, .Src = Right});
    }
}

void emit_test(program_code *code, operand A, operand B) {
    emit(code, (asm_instruction){.Op = ASM_TEST, .Dst = A, .Src = B});
}

void emit_je(program_code *code, string Label) {
    emit(code, (asm_instruction){.Op = ASM_JE, .Dst = LabelOperand(Label)});
}

operand rax_from_size(operand_size Size) {
    switch (Size) {
        case SIZE_8:  return Al;
        case SIZE_16: return Ax;
        case SIZE_32: return Eax;
        case SIZE_64: return Rax;
        default:      assert(false); return (operand){};
    }
}

// The result is stored in the register that the return value points to (Rax)
operand gen_binop(ast_node *node, program_code *code, int depth) {
    token_type Op = node->BinaryOp.Operation;

    operand Left  = gen_expression(node->BinaryOp.Left, code, depth);
    operand Right = gen_expression(node->BinaryOp.Right, code, depth);

    switch (Op) {
        case TOKEN_EQUALS_EQUALS:
        case TOKEN_BANG_EQUALS:   {
            emit_cmp(code, Left, Right);

            emit(code, (asm_instruction){.Op = Op == TOKEN_EQUALS_EQUALS ? ASM_SETE : ASM_SETNE, .Dst = Al});
            emit(code, (asm_instruction){.Op = ASM_MOVZX, .Dst = rax_from_size(Left.Size), .Src = Al});

            return rax_from_size(Left.Size);
        }
        case TOKEN_LESS: {
            emit_cmp(code, Left, Right);
        }
        default: {
            gen_error("Didn't implement this binary operation %s\n", token_name(Op));
            return (operand){};
        }
    }
}

// Emit instructions and generate a resulting operand
operand gen_expression(ast_node *node, program_code *code, int depth) {
    operand Result = {};

    switch (node->Type) {
        case NODE_IDENT: {
            symbol *Sym = node->Ident.Sym;

            Result.Type = OPERAND_MEM;
            Result.Size = Sym->Size;

            if (Sym->Section == SECTION_STACK) {
                Result.Mem.Base   = REG_RBP;
                Result.Mem.Offset = -Sym->StackOffset;
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
            Result.Size      = 4;  // TODO: Is this a good idea?
            Result.Imm.Value = node->IntegerLit.Value;
            break;
        }
        case NODE_STRING_LIT: {
            Result.Type = OPERAND_IMM;
            break;
        }

        case NODE_BINARY_OP: {
            return gen_binop(node, code, depth);
            break;
        }

        default: {
            break;
        }
    }

    return Result;
}

operand scratch_register(operand_size size) {
    switch (size) {
        case SIZE_32: return R11d;
        case SIZE_64: return R11;
        default:      assert(false);
    }

    return (operand){};
}

void emit_mov(program_code *code, operand Dst, operand Src) {
    // assert(Dst.Size >= Src.Size);

    asm_opcode Op = Dst.Size == Src.Size ? ASM_MOV : ASM_MOVZX;

    if (Dst.Type == OPERAND_MEM && Src.Type == OPERAND_MEM) {
        operand Tmp = scratch_register(Dst.Size);
        emit(code, (asm_instruction){.Op = Op, .Dst = Tmp, .Src = Src});
        emit(code, (asm_instruction){.Op = Op, .Dst = Dst, .Src = Tmp});
    } else {
        emit(code, (asm_instruction){.Op = Op, .Dst = Dst, .Src = Src});
    }
}

void gen_statement(ast_node *node, program_code *code, int depth) {
    switch (node->Type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->Program.FunctionCount; i++) {
                gen_statement(node->Program.Functions[i], code, depth);
            }
            break;
        case NODE_FUNC_DEF: {
            ast_node *Name = node->FuncDef.Name;

            code->CurrentFunction           = Name->Ident.Name;
            code->CurrentFunctionReturnSize = get_type_size(node->FuncDef.ReturnType->DataType.Type);

            int local_size = Name->Ident.Sym->Size;

            emit_function_label(code, Name->Ident.Name);
            emit_function_prologue(code, local_size);

            gen_statement(node->FuncDef.Body, code, depth + 1);

            emit_function_epilogue(code);
            break;
        }
        case NODE_BLOCK: {
            for (int i = 0; i < node->Program.FunctionCount; i++) {
                gen_statement(node->Program.Functions[i], code, depth);
            }
            break;
        }
        case NODE_BINARY_OP: {
            switch (node->BinaryOp.Operation) {
                case TOKEN_EQUALS: {
                    operand Src = gen_expression(node->BinaryOp.Right, code, depth);
                    operand Dst = gen_expression(node->BinaryOp.Left, code, depth);

                    emit_mov(code, Dst, Src);
                    break;
                }
                default: {
                    gen_error("Haven't implemented parsing for binary operation of type %c yet, sorry!",
                              node->BinaryOp.Operation);
                    break;
                }
            }
            break;
        }
        case NODE_IF: {
            operand Result = gen_expression(node->If.Condition, code, depth + 1);

            string L_Else = new_label(code);

            // ZF = 0 if Result = 0
            emit_test(code, Result, Result);

            emit_je(code, L_Else);

            gen_statement(node->If.ThenBlock, code, depth);

            emit_label(code, L_Else);

            if (node->If.ElseBlock) gen_statement(node->If.ElseBlock, code, depth);
            break;
        }
        case NODE_FOR: {
            /*
            process_node(node->For.Init, code, depth + 1);
            // TODO: while (node->For.Condition)
            process_node(node->For.Body, code, depth + 1);
            process_node(node->For.Advance, code, depth + 1);
            */
            break;
        }
        case NODE_RETURN: {
            ast_node *Val   = node->Return.Value;
            operand Operand = gen_expression(Val, code, depth);

            // TODO: What if we return a struct (or float)?
            emit_mov(code, rax_from_size(code->CurrentFunctionReturnSize), Operand);

            emit_jump(code, function_end_label(code));
            break;
        }
        case NODE_VAR_DECL: {
            ast_node *Init = node->VarDecl.Init;

            if (!Init) break;

            ast_node *Ident = node->VarDecl.Name;
            symbol *Sym     = Ident->Ident.Sym;

            int Offset = Sym->StackOffset;

            // Variable is stored at [rbp - Offset]
            operand Mem = {
                .Type = OPERAND_MEM,
                .Size = Sym->Size,
                .Mem  = {.Base = REG_RBP, .Index = REG_NONE, .Scale = 0, .Offset = -Offset}
            };

            operand Value = gen_expression(Init, code, depth);

            emit_mov(code, Mem, Value);

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

char *register_name(register_id Reg) {
    switch (Reg) {
        case REG_NONE: break;

        case REG_AL:  return "al";
        case REG_AX:  return "ax";
        case REG_EAX: return "eax";

        case REG_RAX: return "rax";
        case REG_RBX: return "rbx";
        case REG_RCX: return "rcx";
        case REG_RDX: return "rdx";

        case REG_RSI: return "rsi";
        case REG_RDI: return "rdi";

        case REG_RBP: return "rbp";
        case REG_RSP: return "rsp";

        case REG_R8:   return "r8";
        case REG_R9:   return "r9";
        case REG_R10:  return "r10";
        case REG_R11:  return "r11";
        case REG_R11d: return "r11d";
        case REG_R12:  return "r12";
        case REG_R13:  return "r13";
        case REG_R14:  return "r14";
        case REG_R15:  return "r15";
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

void print_mem(FILE *out, operand op) {
    fprintf(out, "%s [%s", size_directive(op.Size), register_name(op.Mem.Base));
    if (op.Mem.Index) fprintf(out, " + %s*%d", register_name(op.Mem.Index), op.Mem.Scale);

    int Off = op.Mem.Offset;

    if (Off >= 0) {
        fprintf(out, " + ");
    } else {
        fprintf(out, " - ");
        Off = -Off;
    }

    fprintf(out, "%d]", Off);
}

void print_operand(FILE *out, operand op) {
    switch (op.Type) {
        case OPERAND_NONE:  fprintf(out, "none"); break;
        case OPERAND_REG:   fprintf(out, "%s", register_name(op.Reg.Register)); break;
        case OPERAND_IMM:   fprintf(out, "%lld", (long long)op.Imm.Value); break;
        case OPERAND_MEM:   print_mem(out, op); break;
        case OPERAND_LABEL: fprintf(out, "%.*s", (int)op.Label.Name.Length, op.Label.Name.Data); break;
        default:            fprintf(out, "operand(?)"); break;
    }
}

char *instruction_name(asm_opcode Opcode) {
    switch (Opcode) {
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

void print_instruction(FILE *out, asm_instruction *in) {
    switch (in->Op) {
        case ASM_LABEL: {
            string_print_to(out, in->Dst.Label.Name);
            fprintf(out, ":");
            break;
        }
        default: {
            fprintf(out, "%s ", instruction_name(in->Op));

            if (in->Op == ASM_RET) break;

            print_operand(out, in->Dst);

            if (in->Op == ASM_JE || in->Op == ASM_JMP || in->Op == ASM_PUSH || in->Op == ASM_POP ||
                in->Op == ASM_SETNE || in->Op == ASM_SETE)
                break;

            fprintf(out, ", ");
            print_operand(out, in->Src);
            break;
        }
    }

    fputc('\n', out);
}

program_code gen_program_code(FILE *out, memory_arena *arena, ast_node *ast) {
    program_code Code = {};

    Code.InstructionArena = make_arena();
    Code.GeneralArena     = arena;

    gen_statement(ast, &Code, 0);

    asm_instruction *Instructions = (asm_instruction *)Code.InstructionArena.Data;

    fprintf(out, ".intel_syntax noprefix\n.global main\n.type main, @function\n\n");

    for (int i = 0; i < Code.InstructionCount; i++) {
        asm_instruction *Instr = Instructions + i;

        if (Instr->Op != ASM_LABEL) fprintf(out, "  ");

        print_instruction(out, Instr);
    }

    return Code;
}

void free_program_code(program_code *program) { free_arena(&program->InstructionArena); }
