#include "gen.h"

#include <assert.h>
#include <string.h>

#include "parser.h"
#include "util/util.h"

#define Imm(value) \
    (operand) { .Type = OPERAND_IMM, .Imm.Value = value }

constexpr operand Rbp = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_RBP};
constexpr operand Rsp = (operand){.Type = OPERAND_REG, .Size = SIZE_64, .Reg.Register = REG_RSP};

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
    asm_instruction Instructions[] = {
        {.Op = ASM_PUSH, .Dst = Rbp},
        {.Op = ASM_MOV, .Dst = Rbp, .Src = Rsp},
        {.Op = ASM_SUB, .Dst = Rsp, .Src = Imm(local_size)},
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

int calculate_function_local_size(ast_node *Function) {
    int Size = 0;

    /*
    ast_node *Params = Function->FuncDef.Params;
    int ParamCount = Function->FuncDef.ParamCount;

    for (int i = 0; i < ParamCount; i++) {
        Size += Params[i].VarDecl.Size;
    }

    ast_node *Block = Function->FuncDef.Body;

    int Count             = Block->Block.StatementCount;
    ast_node **Statements = Block->Block.Statements;

    for (int i = 0; i < Count; i++) {
        ast_node *Node = Statements[i];

        if (Node->Type == NODE_VAR_DECL) {
            Size += Node->VarDecl.Size;
        }
    }
    */

    return Size;
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

            // int block_locals_size = calculate_local_function_size(Body);

            size_t Count          = Body->Block.StatementCount;
            ast_node **Statements = Body->Block.Statements;

            emit_next_label(code);
            emit_function_prologue(code, 0);

            //assert(code->Symbols[depth + 1].SymbolCount == 0);

            for (size_t i = 0; i < Count; i++) {
                process_node(Statements[i], code, depth + 1);
            }

            emit_function_epilogue(code);

            //code->Symbols[depth + 1].SymbolCount = 0;
            break;
        }
        case NODE_FOR: {
            process_node(node->For.Init, code, depth + 1);
            // TODO: while (node->For.Condition)
            process_node(node->For.Body, code, depth + 1);
            process_node(node->For.Advance, code, depth + 1);

            //code->Symbols[depth + 1].SymbolCount = 0;
            break;
        }
        case NODE_VAR_DECL: {
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

program_code gen_program_code(memory_arena *arena, ast_node *ast) {
    program_code Code = {};

    Code.InstructionArena = make_arena();
    Code.GeneralArena     = arena;

    process_node(ast, &Code, 0);

    return Code;
}

void free_program_code(program_code *program) { free_arena(&program->InstructionArena); }
