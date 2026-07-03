#include "gen.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "parser.h"
#include "util/util.h"

#define Imm(value) \
    (operand) { .Type = OPERAND_IMM, .Imm.Value = value }

constexpr operand Eax = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_EAX};
constexpr operand Rax = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_RAX};
constexpr operand Rbp = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_RBP};
constexpr operand Rsp = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_RSP};

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

void emit_next_label(program_code *code) {
    string Name = string_make(code->GeneralArena, 10);

    Name.Data[0] = 'L';
    Name.Length++;

    Name.Length += sprintf(Name.Data + 1, "%d", code->Label++);

    emit(code, (asm_instruction){.Op = ASM_LABEL, .Dst = (operand){.Label.Name = Name}});
}

void emit_function_label(program_code *code, string Name) {
    emit(code, (asm_instruction){.Op = ASM_LABEL, .Dst = (operand){.Label.Name = Name}});
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

void emit_function_epilogue(program_code *code) {
    asm_instruction Instructions[] = {
        {.Op = ASM_MOV, .Dst = Rsp, .Src = Rbp},
        {.Op = ASM_POP, .Dst = Rbp},
        {.Op = ASM_RET},
    };

    emit_count(code, Instructions, ArraySize(Instructions));
}

operand gen_expression(ast_node *node) {
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
            break;
        }

        default: {
            break;
        }
    }

    return Result;
}

// TODO: Use r11?
operand scratch_register(operand_size size) {
    switch (size) {
        case SIZE_32: return Eax;
        case SIZE_64: return Rax;
        default:      assert(false);
    }

    return (operand){};
}

void emit_mov(program_code *code, operand Dst, operand Src) {
    if (Dst.Type == OPERAND_MEM && Src.Type == OPERAND_MEM) {
        operand Tmp = scratch_register(Dst.Size);
        emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Tmp, .Src = Src});
        emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Dst, .Src = Tmp});
    } else {
        emit(code, (asm_instruction){.Op = ASM_MOV, .Dst = Dst, .Src = Src});
    }
}

void process_node(ast_node *node, program_code *code, int depth) {
    switch (node->Type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->Program.FunctionCount; i++) {
                process_node(node->Program.Functions[i], code, depth);
            }
            break;
        case NODE_FUNC_DEF: {
            ast_node *Body = node->FuncDef.Body;

            int local_size = node->FuncDef.Name->Ident.Sym->Size;

            size_t Count          = Body->Block.StatementCount;
            ast_node **Statements = Body->Block.Statements;

            emit_function_label(code, node->FuncDef.Name->Ident.Name);
            emit_function_prologue(code, local_size);

            for (size_t i = 0; i < Count; i++) {
                process_node(Statements[i], code, depth + 1);
            }

            emit_function_epilogue(code);

            // code->Symbols[depth + 1].SymbolCount = 0;
            break;
        }
        case NODE_BINARY_OP: {
            switch (node->BinaryOp.Operation) {
                case TOKEN_EQUALS: {
                    operand Src = gen_expression(node->BinaryOp.Right);
                    operand Dst = gen_expression(node->BinaryOp.Left);

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
            operand Operand = gen_expression(Val);

            emit_mov(code, Rax, Operand);
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

            operand Value = gen_expression(Init);

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

        case REG_EAX: return "eax";

        case REG_RAX: return "rax";
        case REG_RBX: return "rbx";
        case REG_RCX: return "rcx";
        case REG_RDX: return "rdx";

        case REG_RSI: return "rsi";
        case REG_RDI: return "rdi";

        case REG_RBP: return "rbp";
        case REG_RSP: return "rsp";

        case REG_R8:  return "r8";
        case REG_R9:  return "r9";
        case REG_R10: return "r10";
        case REG_R11: return "r11";
        case REG_R12: return "r12";
        case REG_R13: return "r13";
        case REG_R14: return "r14";
        case REG_R15: return "r15";
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
        case OPERAND_LABEL: fprintf(out, "%.*s:", (int)op.Label.Name.Length, op.Label.Name.Data); break;
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

            if (in->Op == ASM_PUSH || in->Op == ASM_POP) break;

            fprintf(out, ", ");
            print_operand(out, in->Src);
            break;
        }
    }

    fputc('\n', out);
}

program_code gen_program_code(memory_arena *arena, ast_node *ast) {
    program_code Code = {};

    Code.InstructionArena = make_arena();
    Code.GeneralArena     = arena;

    process_node(ast, &Code, 0);

    asm_instruction *Instructions = (asm_instruction *)Code.InstructionArena.Data;

    FILE *out = stdout;

    printf("\n--asm--\n");

    for (int i = 0; i < Code.InstructionCount; i++) {
        asm_instruction *Instr = Instructions + i;

        if (Instr->Op != ASM_LABEL) fprintf(out, "  ");

        print_instruction(out, Instr);
    }

    printf("--end asm--\n");

    return Code;
}

void free_program_code(program_code *program) { free_arena(&program->InstructionArena); }
