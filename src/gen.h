#pragma once

#include <stdint.h>

#include "parser.h"
#include "util/arena.h"

#define MAX_FUNCTIONS 1024
#define MAX_VARS 16384
#define MAX_STRING_LIT 16384

typedef enum {
    ASM_INVALID,
    ASM_LABEL,
    ASM_MOV,
    ASM_LEA,
    ASM_AND,
    ASM_OR,
    ASM_XOR,
    ASM_NOT,
    ASM_SHL,
    ASM_SHR,
    ASM_SAR,
    ASM_CDQ,
    ASM_MOVSX,
    ASM_NOP,
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
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSI,
    REG_RDI,
    REG_RBP,
    REG_RSP,
    REG_RIP,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
} register_id;

typedef enum {
    DISPLACEMENT_NONE,
    DISPLACEMENT_CONSTANT,
    DISPLACEMENT_LABEL,
} displacement_type;

typedef struct {
    displacement_type Type;

    union {
        int32_t Value;
        string Label;
    };
} displacement;

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
            bool IsAddress; // If true, LEA, otherwise MOV
            register_id Base;
            register_id Index;
            int Scale;
            displacement Displacement;
        } Mem;

        struct {
            string Label;
        } StringLit;

        struct {
            string Name;
        } Label;
    };
} operand;

typedef struct {
    asm_opcode Op;
    operand Dst, Src;
} asm_instruction;

typedef enum {
    RODATA_STRING_LIT,
} rodata_entry_type;

typedef struct {
    rodata_entry_type Type;
    string Label;

    union {
        string StringLit;
    };
} rodata_entry;

typedef struct {
    memory_arena InstructionArena;
    memory_arena *GeneralArena;

    string CurrentFunction;
    operand_size CurrentFunctionReturnSize;

    rodata_entry *RodataEntries;
    int RodataEntryCount;

    size_t InstructionCount;

    int Label;
    int CurrentRodataLabel;
    string CurrentBreakLabel;
} program_code;

program_code gen_program_code(FILE *out, memory_arena *arena, ast_node *ast);
void free_program_code(program_code *program);
void print_instruction(memory_arena *Arena, FILE *out, asm_instruction *in);
operand emit_expression(ast_node *node, program_code *code);
operand scratch_register(operand_size size);
void free_scratch_register(operand Op);
void emit_move(program_code *code, operand Dst, operand Src);
