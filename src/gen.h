#pragma once

#include <stdint.h>

#include "parser.h"
#include "util/arena.h"

#define MAX_FUNCTIONS 1024
#define MAX_VARS 16384

typedef enum {
    ASM_LABEL,
    ASM_MOV,
    ASM_LEA,
    ASM_AND,
    ASM_OR,
    ASM_XOR,
    ASM_NOT,
    ASM_SHL,
    ASM_SHR,
    ASM_SAR,    // arithmetic right shift, important for signed types
    ASM_CDQ,    // sign-extend rax into rdx:rax before idiv
    ASM_MOVSX,  // sign-extending move, counterpart to your MOVZX
    ASM_NOP,    // useful for debugging
    ASM_PUSH,
    ASM_POP,
    ASM_ADD,
    ASM_SUB,
    ASM_IMUL,
    ASM_IDIV,
    ASM_NEG,
    ASM_CMP,
    ASM_TEST,
    ASM_SETE,
    ASM_SETNE,
    ASM_SETL,
    ASM_SETLE,
    ASM_SETG,
    ASM_SETGE,
    ASM_MOVZX,
    ASM_JMP,
    ASM_JE,
    ASM_JNE,
    ASM_JL,
    ASM_JLE,
    ASM_JG,
    ASM_JGE,
    ASM_CALL,
    ASM_RET,
    ASM_SYSCALL
} asm_opcode;

typedef enum {
    OPERAND_NONE = 0,
    OPERAND_REG,
    OPERAND_IMM,
    OPERAND_MEM,
    OPERAND_LABEL,
} operand_type;

typedef enum { SIZE_NONE = 0, SIZE_8 = 1, SIZE_16 = 2, SIZE_32 = 4, SIZE_64 = 8 } operand_size;

typedef enum {
    REG_NONE = 0,

    REG_AL,
    REG_AX,
    REG_EAX,

    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,

    REG_RSI,
    REG_RDI,

    REG_RBP,
    REG_RSP,

    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R11d,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
} register_id;

typedef struct {
    operand_type Type;
    operand_size Size;

    union {
        struct {
            register_id Register;
        } Reg;

        struct {
            int64_t Value;
        } Imm;

        struct {
            register_id Base;
            register_id Index;
            int Scale;
            int32_t Offset;
        } Mem;

        struct {
            string Name;
        } Label;
    };
} operand;

typedef struct {
    asm_opcode Op;
    operand Dst, Src;
} asm_instruction;

typedef struct {
    string Label;
    uint8_t *Data;
    size_t Size;
    bool ReadOnly;
} data_entry;

typedef struct {
    memory_arena InstructionArena;
    memory_arena *GeneralArena;

    string CurrentFunction;
    int CurrentFunctionReturnSize;

    size_t InstructionCount;

    int Label;

    data_entry *DataEntries;
    size_t DataEntryCount;
} program_code;

program_code gen_program_code(FILE *out, memory_arena *arena, ast_node *ast);
void free_program_code(program_code *program);
void print_instruction(FILE *out, asm_instruction *in);
operand gen_expression(ast_node *node, program_code *code, int depth);
operand scratch_register(operand_size size);
