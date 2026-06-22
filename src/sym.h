#pragma once

#include "util/string.h"

typedef enum {
    SYM_VAR,
    SYM_FUNC,
    SYM_STRUCT,
    SYM_FIELD,
} symbol_type;

typedef enum {
    SECTION_STACK,
    SECTION_BSS,
    SECTION_DATA,
} section;

typedef struct {
    string Name;
    symbol_type Type;
    section Section;
    int Size;

    // Storage information
    union {
        int StackOffset;
        int FieldOffset;
    };
} symbol;

struct ast_node;

int resolve_type_size(struct ast_node *type);
struct ast_node *get_node_identifier(struct ast_node *node);
