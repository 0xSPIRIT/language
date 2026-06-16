#pragma once

#include <stdint.h>

#include "parser.h"
#include "util/arena.h"

#define MAX_FUNCTIONS 1024
#define MAX_VARS 16384

typedef struct {
    void *Address;
    string Name;
} function;

typedef struct {
    memory_arena Arena;
    string ProgramCode;

    function Functions[MAX_FUNCTIONS];
} program_code;

program_code gen_program_code(ast_node *ast);
void free_program_code(program_code *program);
