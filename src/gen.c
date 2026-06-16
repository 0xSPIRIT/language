#include "gen.h"

#include <string.h>

#include "parser.h"

#define Emit(instr) emit_instruction(CSTR(instr), code)

void emit_instruction(string instruction, program_code *code) {
    char *Buffer = arena_push(&code->Arena, instruction.Length + 1);
    memcpy(Buffer, instruction.Data, instruction.Length);
    code->ProgramCode.Length += instruction.Length + 1;
    Buffer[instruction.Length] = '\n';
}

void process_node(ast_node *node, program_code *code) {
    switch (node->Type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->Program.FunctionCount; i++) {
                process_node(node->Program.Functions[i], code);
            }

            for (int i = 0; i < node->Program.GlobalVarCount; i++) {
                process_node(node->Program.GlobalVars[i], code);
            }
            break;
        case NODE_FUNC_DEF: break;
        case NODE_VAR_DECL: break;
        default:
            printf("Haven't implemented codegen for node type ");
            print_node_type(node);
            printf("\n");
            break;
    }
}

void sample(program_code *code) {
    Emit(".section .text");
    Emit(".global _start");
    Emit("_start:");
    Emit("mov $60, %rax");
    Emit("mov $67, %rdi");
    Emit("syscall");
}

program_code gen_program_code(ast_node *ast) {
    program_code Code = {};

    Code.Arena            = make_arena();
    Code.ProgramCode.Data = Code.Arena.Data;

    sample(&Code);

    return Code;
}

void free_program_code(program_code *program) { free_arena(&program->Arena); }
