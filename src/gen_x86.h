#pragma once

#include <stdint.h>

#include "parser.h"
#include "util/arena.h"

typedef struct {
    memory_arena Arena;
    uint8_t *Data;
    uint32_t DataSize;
} program_code;

program_code gen_x86(ast_node *ast);
void free_x86(program_code *program);
