#pragma once

#include "type.h"
#include "util/string.h"

constexpr int MAX_SCOPES  = 16384;
constexpr int MAX_SYMBOLS = 16384;  // per scope
constexpr int MAX_FIELDS  = 16384;

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
    SECTION_REG,
} section;

typedef struct {
    struct symbol **Fields;
    int FieldCount;
} struct_data;

typedef struct {
    struct symbol **Params;
    int ParamCount;
    type ReturnType;
} func_data;

typedef struct symbol {
    string Name;
    symbol_type Type;
    struct symbol *StructType;  // nullable. non-null if symbol is SYM_VAR whose type is a SYM_STRUCT
    section Section;
    int Size;
    int PointerDepth;

    // Storage information
    union {
        int StackOffset;
        int FieldOffset;
        int ParamIndex;
        struct_data Structure;
        func_data Function;
    };
} symbol;

typedef struct symbols_scope {
    symbol **Symbols;
    size_t SymbolCount;

    struct symbols_scope *Parent; // Parent scope.
} symbols_scope;

typedef struct {
    symbols_scope *Scopes;
    symbols_scope *CurrentScope;
    int Count;
} scopes;

struct ast_node;

int resolve_type_size(struct ast_node *type);
struct ast_node *get_node_identifier(struct ast_node *node);
void _resolve_symbols(memory_arena *arena,
                      struct ast_node *node,
                      scopes *Scopes,
                      symbol current_symbol,
                      bool must_exist);
int get_type_size(type t);
symbols_scope *push_scope(memory_arena *Arena, scopes *Scopes);
void pop_scope(scopes *Scopes);
