#include "sym.h"

#include <assert.h>
#include <string.h>

#include "parser.h"
#include "util/arena.h"

constexpr int MAX_DEPTH   = 512;
constexpr int MAX_SYMBOLS = 16384;

typedef struct {
    symbol *Symbols;
    size_t SymbolCount;
} symbols_scope;

typedef struct {
    symbols_scope *Symbols;
} symbol_resolver;

int get_type_size(type t) {
    switch (t) {
        case TYPE_S8:     return 1;
        case TYPE_S16:    return 2;
        case TYPE_S32:    return 4;
        case TYPE_S64:    return 8;
        case TYPE_U8:     return 1;
        case TYPE_U16:    return 2;
        case TYPE_U32:    return 4;
        case TYPE_U64:    return 8;
        case TYPE_FLOAT:  return 4;
        case TYPE_VOID:   return 0;
        case TYPE_PTR:    return 8;
        case TYPE_ARRAY:  return 8;
        case TYPE_STRUCT: return -1;
    }

    assert(false);

    return -1;
}

int resolve_struct_size(ast_node *Node) {
    int Size = 0;

    for (int i = 0; i < Node->Struct.FieldCount; i++) {
        int FieldSize = resolve_type_size(Node->Struct.Fields[i]->VarDecl.Type);

        if (FieldSize == -1) {
            parse_error(0, "Couldn't resolve type %s\n", Node->Struct.Fields[i]->VarDecl.Name);
        }

        Size += FieldSize;
    }

    return Size;
}

int resolve_type_size(ast_node *type) {
    if (type->VarDecl.Type->DataType.Type == TYPE_STRUCT) return resolve_struct_size(type);

    return get_type_size(type->VarDecl.Type->DataType.Type);
}

// Returns address of symbol in scope array
symbol *push_symbol(symbols_scope *Scope, symbol Sym) {
    Scope->Symbols[Scope->SymbolCount] = Sym;
    return &Scope->Symbols[Scope->SymbolCount++];
}

// Returns NULL when not exist
symbol *search_for_symbol(string name, symbols_scope *Scopes, int depth) {
    for (int i = depth; i >= 0; i--) {
        symbols_scope *Scope = Scopes + i;

        for (int j = 0; j < Scope->SymbolCount; j++) {
            if (string_equals(Scope->Symbols[j].Name, name)) {
                return &Scope->Symbols[j];
            }
        }
    }

    return NULL;
}

// Gets the identifier name associated with a node
ast_node *get_node_identifier(ast_node *node) {
    switch (node->Type) {
        case NODE_VAR_DECL: return node->VarDecl.Name;
        case NODE_FUNC_DEF: return node->FuncDef.Name;
        case NODE_CALL:     return node->Call.FuncName;
        case NODE_STRUCT:   return node->Struct.Name;
        case NODE_IDENT:    return node;
        default:            return NULL;
    }
}

void resolve(ast_node *node, symbol *Sym) { get_node_identifier(node)->Ident.Sym = Sym; }

void _resolve_symbols(ast_node *node, symbols_scope *Scopes, int depth, symbol current_symbol) {
    if (!node) return;

    symbols_scope *CurrentScope = Scopes + depth;

    switch (node->Type) {
        case NODE_IDENT: {
            string Name      = node->Ident.Name;
            symbol *Existing = search_for_symbol(Name, Scopes, depth);

            current_symbol.Name = Name;

            resolve(node, Existing ? Existing : push_symbol(CurrentScope, current_symbol));
            break;
        }
        case NODE_PROGRAM: {
            for (int i = 0; i < node->Program.FunctionCount; i++)
                _resolve_symbols(node->Program.Functions[i], Scopes, depth, (symbol){});

            for (int i = 0; i < node->Program.GlobalDeclCount; i++)
                _resolve_symbols(node->Program.GlobalDecls[i], Scopes, depth, (symbol){});
            break;
        }
        case NODE_VAR_DECL: {
            symbol Sym = {.Type = SYM_VAR, .StackOffset = resolve_type_size(node)};

            _resolve_symbols(node->VarDecl.Name, Scopes, depth, Sym);

            for (int i = 0; i < node->VarDecl.ChildDeclsCount; i++) {
                _resolve_symbols(node->VarDecl.ChildDecls[i], Scopes, depth, (symbol){});
            }
            break;
        }
        case NODE_FUNC_DEF: {
            symbol Sym = {.Type = SYM_FUNC};

            _resolve_symbols(node->FuncDef.Name, Scopes, depth, Sym);
            break;
        }
        case NODE_CALL: {
            symbol Sym = {.Type = SYM_FUNC};

            _resolve_symbols(node->Call.FuncName, Scopes, depth, Sym);
            break;
        }
        case NODE_BINARY_OP: {
            // This is a little weird. We need to think about this a bit more.

            symbol A = {.Type = SYM_VAR};

            symbol B = {.Type = (node->BinaryOp.Operation == TOKEN_DOT) ? SYM_FIELD : SYM_VAR};

            if (B.Type == SYM_FIELD) {
                // You need to get the offset & size
            }

            _resolve_symbols(node->BinaryOp.Right, Scopes, depth, B);
            _resolve_symbols(node->BinaryOp.Left, Scopes, depth, A);
            break;
        }
        case NODE_STRUCT: {
            symbol Sym = {.Type = SYM_FIELD};

            for (int i = 0; i < node->Struct.FieldCount; i++) {

            }
            break;
        }
        default: {
            break;
        }
    }
}

symbols_scope make_symbols_scope(memory_arena *arena) {
    symbols_scope Scope = {};

    Scope.Symbols     = arena_push(arena, sizeof(ast_node) * MAX_SYMBOLS);
    Scope.SymbolCount = 0;

    return Scope;
}

void resolve_symbols(ast_node *ast) {
    memory_arena arena = make_arena();

    symbols_scope *Scopes = arena_push(&arena, sizeof(symbols_scope) * MAX_DEPTH);

    for (int i = 0; i < MAX_DEPTH; i++) {
        Scopes[i] = make_symbols_scope(&arena);
    }

    _resolve_symbols(ast, Scopes, 0, (symbol){});
}
