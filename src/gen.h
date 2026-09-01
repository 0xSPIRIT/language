#pragma once

#include <stdint.h>

#include "parser.h"
#include "util/arena.h"

#define MAX_FUNCTIONS 1024
#define MAX_VARS 16384
#define MAX_SECTION_ENTRIES 16384
#define MAX_DEPTH 16384

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
    int Size;

    union {
        struct {
            register_id Register;
        } Reg;

        struct {
            int64_t Value;
        } Imm;

        struct {
            bool IsAddress;  // If true, LEA, otherwise MOV
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
    STRING_LIT,
    INT_LIT,
    STRUCT,
} section_entry_type;

typedef struct {
    section_entry_type Type;

    string Name;
    int Size;

    union {
        string StringLit;
        uint64_t IntegerLit;
    };
} section_entry;

typedef struct {
    string BreakLabel, ContinueLabel;
} loop;

typedef struct {
    memory_arena InstructionArena;
    memory_arena *GeneralArena;

    // We store nested functions here which are queued up to be emitted at global scope.
    ast_node **FunctionQueue;
    int FunctionQueueSize;

    string CurrentFunction;
    int CurrentFunctionReturnSize;

    section_entry *RodataEntries;
    int RodataEntryCount;

    section_entry *DataEntries;
    int DataEntryCount;

    section_entry *BssEntries;
    int BssEntryCount;

    size_t InstructionCount;

    int Label;
    int CurrentRodataLabel;

    loop *Loops;
    int LoopCount;
} program_code;

program_code gen_program_code(FILE *out, memory_arena *arena, ast_node *ast);
void free_program_code(program_code *program);
void print_instruction(FILE *out, asm_instruction *in);
void emit_statement(ast_node *node, program_code *code);
operand emit_expression(ast_node *node, program_code *code);
operand scratch_register(int size);
void free_scratch_register();
void emit_move(program_code *code, operand Dst, operand Src);
bool is_pointer_math_op(program_code *code, ast_node *Node);
operand emit_pointer_math_op(program_code *code, ast_node *Operand, int *OutSize);
void emit_free_all_scratch_registers(program_code *code);
operand emit_dereference(program_code *code, ast_node *OperandNode);
type_info get_type_info_from_operand(ast_node *Node, bool Principal);

void dbg_operand(operand op);
void dbg_instr(asm_instruction *in);
