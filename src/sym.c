#include "sym.h"

#include <assert.h>
#include <string.h>

#include "parser.h"
#include "util/arena.h"

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
symbol *push_symbol(memory_arena *arena, symbols_scope *Scope, symbol Sym) {
    symbol *NewSym = arena_push(arena, sizeof(symbol));
    *NewSym        = Sym;

    Scope->Symbols[Scope->SymbolCount++] = NewSym;

    return NewSym;
}

// Pushes symbol to struct
symbol *push_field_symbol_to_struct(memory_arena *arena, symbol *structure, symbol Sym) {
    symbol *SymLoc = arena_push(arena, sizeof(symbol));

    assert(SymLoc);

    *SymLoc = Sym;

    structure->Structure.Fields[structure->Structure.FieldCount++] = SymLoc;

    return SymLoc;
}

struct_data make_structure_data(memory_arena *arena, int max_fields) {
    struct_data Result = {};

    Result.Fields = arena_push(arena, max_fields * sizeof(symbol *));
    return Result;
}

// Returns NULL when not exist
symbol *search_for_symbol(string name, symbols_scope *Scopes, int depth) {
    for (int i = depth; i >= 0; i--) {
        symbols_scope *Scope = Scopes + i;

        for (int j = 0; j < Scope->SymbolCount; j++) {
            if (string_equals(Scope->Symbols[j]->Name, name)) {
                return Scope->Symbols[j];
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

// Traverses all pointingto's on a type and returns the leaf node.
ast_node *get_leaf_type(ast_node *type) {
    assert(type->Type == NODE_TYPE);

    if (!type->DataType.PointingTo) return type;

    return get_leaf_type(type->DataType.PointingTo);
}

symbol *resolve_struct_symbol(ast_node *StructType, symbols_scope *Scopes, int current_depth) {
    string TypeName = StructType->DataType.Name->Ident.Name;
    symbol *Sym     = search_for_symbol(TypeName, Scopes, current_depth);

    if (Sym) {
        if (Sym->Type == SYM_STRUCT) {
            StructType->DataType.Name->Ident.Sym = Sym;
            return Sym;
        } else {
            parse_error(0, "The struct type couldn't be resolved because there was another symbol with the same name.");
        }
    } else {
        parse_error(0, "We tried to resolve a struct type in a field but it was not declared yet.");
    }

    assert(false);

    return 0;
}

symbol *search_struct_for_field(symbol *structure, string Name) {
    for (int i = 0; i < structure->Structure.FieldCount; i++) {
        symbol *Field = structure->Structure.Fields[i];

        if (string_equals(Field->Name, Name)) return Field;
    }

    return NULL;
}

symbol make_var_decl_symbol_and_resolve_type(memory_arena *arena,
                                             symbols_scope *Scopes,
                                             int depth,
                                             ast_node *var_decl) {
    ast_node *Type   = var_decl->VarDecl.Type;
    type DataType    = Type->DataType.Type;
    string FieldName = var_decl->VarDecl.Name->Ident.Name;

    symbol DeclSym = {
        .Name = FieldName,
        .Type = SYM_VAR,
    };

    // Calculate pointer depth
    if (Type) {
        ast_node *TypeNode = Type;
        int Depth          = 0;

        while (TypeNode->DataType.Type == TYPE_PTR || TypeNode->DataType.Type == TYPE_ARRAY) {
            Depth++;
            TypeNode = TypeNode->DataType.PointingTo;
        }

        DeclSym.PointerDepth = Depth;
    }

    // If the field type is another struct,
    // we search for that struct symbol, and get its size.

    if (DataType == TYPE_STRUCT) {
        symbol *StructRef = resolve_struct_symbol(Type, Scopes, depth);

        if (StructRef) {
            DeclSym.Size += StructRef->Size;
            DeclSym.StructType = StructRef;
        }
    } else {
        int ArraySize = 1;

        int TypeSize = get_type_size(DataType);

        if (DataType == TYPE_ARRAY) {
            ArraySize = Type->DataType.ArraySize;

            ast_node *Next = Type->DataType.PointingTo;

            if (Next->DataType.Type == TYPE_STRUCT) {
                symbol *StructRef = resolve_struct_symbol(Next, Scopes, depth);

                if (StructRef) {
                    TypeSize = StructRef->Size;
                }
            } else {
                TypeSize = get_type_size(Next->DataType.Type);
            }
        }

        DeclSym.Size = ArraySize * TypeSize;

        if (DeclSym.Size <= 0) {
            parse_error(0, "A field type cannot be void or have a negative size.");
        }
    }

    return DeclSym;
}

void resolve_var_decl(memory_arena *arena, symbols_scope *Scopes, int depth, ast_node *VarDecl) {
    symbol Sym = make_var_decl_symbol_and_resolve_type(arena, Scopes, depth, VarDecl);

    _resolve_symbols(arena, VarDecl->VarDecl.Name, Scopes, depth, Sym, false);

    for (int i = 0; i < VarDecl->VarDecl.ChildDeclsCount; i++) {
        resolve_var_decl(arena, Scopes, depth, VarDecl->VarDecl.ChildDecls[i]);
    }
}

// Returns the start of the parameter list
symbol **resolve_parameters(memory_arena *arena, ast_node *func, symbols_scope *Scopes, int depth) {
    symbol **Result = Scopes[depth].Symbols + Scopes[depth].SymbolCount;

    for (int i = 0; i < func->FuncDef.ParamCount; i++) {
        ast_node *Param = func->FuncDef.Params[i];
        resolve_var_decl(arena, Scopes, depth, Param);
    }

    return Result;
}

// Returns size of all locals in the function.
int resolve_stack_offsets(ast_node *block, int Offset) {
    for (int i = 0; i < block->Block.StatementCount; i++) {
        ast_node *Stmt = block->Block.Statements[i];

        if (Stmt->Type == NODE_BLOCK) {
            Offset = resolve_stack_offsets(Stmt, Offset);
        } else if (Stmt->Type == NODE_VAR_DECL) {
            ast_node *Identifier = Stmt->VarDecl.Name;
            symbol *Sym          = Identifier->Ident.Sym;

            Sym->StackOffset = Offset;
            Offset += Sym->Size;

            /*
            printf("Name: %.*s Offset: %d\n",
                   (int)Identifier->Ident.Name.Length,
                   Identifier->Ident.Name.Data,
                   Sym->StackOffset);
                   */
        }
    }

    return Offset;
}

// Returns the struct that this particular expression resolves to.
// It also resolves all the identifiers it sees.
// eg. entity.enemy_reference, where enemy_reference is an Enemy,
//     will return the symbol for the type of Enemy.
symbol *resolve_struct_symbol_from_expr(ast_node *node, symbols_scope *Scopes, int depth) {
    if (node->Type == NODE_IDENT) {
        symbol *NodeSym   = search_for_symbol(node->Ident.Name, Scopes, depth);
        symbol *StructRef = NodeSym->StructType;

        node->Ident.Sym = NodeSym;
        return StructRef;
    } else if (node->Type == NODE_BINARY_OP) {
        if (node->BinaryOp.Operation != TOKEN_DOT) {
            parse_error(0, "Trying to do a binary operation that resolves to a structure? What are you doing man?!");
        } else {
            symbol *LeftStruct = resolve_struct_symbol_from_expr(node->BinaryOp.Left, Scopes, depth);

            if (node->BinaryOp.Right->Type != NODE_IDENT) {
                parse_error(0, "Trying to access a struct with a token that isn't an identifier.");
            } else {
                string FieldName                = node->BinaryOp.Right->Ident.Name;
                symbol *FieldSym                = search_struct_for_field(LeftStruct, FieldName);
                node->BinaryOp.Right->Ident.Sym = FieldSym;

                if (!FieldSym) {
                    parse_error(0, "Couldn't find field in struct.");
                } else {
                    symbol *StructRef = FieldSym->StructType;

                    return StructRef;
                }
            }
        }
    }

    return NULL;
}

// Returns the size of the field
// structure is the symbol where the resulting field symbols are placed into
int resolve_struct_decl_field(
    memory_arena *arena, symbols_scope *Scopes, int current_depth, symbol *structure, ast_node *var_decl) {
    symbol FieldSym = make_var_decl_symbol_and_resolve_type(arena, Scopes, current_depth, var_decl);

    FieldSym.Type        = SYM_FIELD;
    FieldSym.FieldOffset = structure->Size;

    symbol *CreatedSymbol = push_field_symbol_to_struct(arena, structure, FieldSym);

    // Point the identifier to the created field symbol.
    var_decl->VarDecl.Name->Ident.Sym = CreatedSymbol;

    return FieldSym.Size;
}

void _resolve_symbols(
    memory_arena *arena, ast_node *node, symbols_scope *Scopes, int depth, symbol current_symbol, bool must_exist) {
    if (!node) return;

    symbols_scope *CurrentScope = Scopes + depth;

    switch (node->Type) {
        case NODE_IDENT: {
            string Name      = node->Ident.Name;
            symbol *Existing = search_for_symbol(Name, Scopes, depth);

            if (must_exist && !Existing) {
                parse_error(0, "Symbol should exist!");
                break;
            }

            current_symbol.Name = Name;

            resolve(node, Existing ? Existing : push_symbol(arena, CurrentScope, current_symbol));
            break;
        }
        case NODE_PROGRAM: {
            for (int i = 0; i < node->Program.GlobalDeclCount; i++)
                _resolve_symbols(arena, node->Program.GlobalDecls[i], Scopes, depth, (symbol){}, false);

            for (int i = 0; i < node->Program.FunctionCount; i++)
                _resolve_symbols(arena, node->Program.Functions[i], Scopes, depth, (symbol){}, false);

            break;
        }
        case NODE_TYPE: {
            if (node->DataType.Type == TYPE_STRUCT)
                _resolve_symbols(arena, node->DataType.Name, Scopes, depth, current_symbol, false);
            break;
        }
        case NODE_VAR_DECL: {
            resolve_var_decl(arena, Scopes, depth, node);
            break;
        }
        case NODE_BLOCK: {
            for (int i = 0; i < node->Block.StatementCount; i++) {
                ast_node *Statement = node->Block.Statements[i];
                _resolve_symbols(arena, Statement, Scopes, depth + 1, (symbol){}, false);
            }
            break;
        }
        case NODE_FUNC_DEF: {
            symbol Sym = {.Type = SYM_FUNC};

            symbol **Params = resolve_parameters(arena, node, Scopes, depth + 1);

            func_data FuncData = {};

            FuncData.ParamCount = node->FuncDef.ParamCount;
            FuncData.Params     = Params;
            FuncData.ReturnType = node->FuncDef.ReturnType->DataType.Type;

            Sym.Function = FuncData;

            _resolve_symbols(arena, node->FuncDef.Name, Scopes, depth, Sym, false);
            _resolve_symbols(arena, node->FuncDef.Body, Scopes, depth, (symbol){}, false);
            _resolve_symbols(arena, node->FuncDef.ReturnType, Scopes, depth, (symbol){}, true);

            // Calculate stack offsets & function's local_size (used for function prologue)
            Sym.Size = resolve_stack_offsets(node->FuncDef.Body, 0);
            break;
        }
        case NODE_CALL: {
            symbol Sym = {.Type = SYM_FUNC};

            _resolve_symbols(arena, node->Call.FuncName, Scopes, depth, Sym, true);

            for (int i = 0; i < node->Call.ArgCount; i++) {
                ast_node *Arg = node->Call.Args[i];

                _resolve_symbols(arena, Arg, Scopes, depth, Sym, true);
            }
            break;
        }
        case NODE_RETURN: {
            _resolve_symbols(arena, node->Return.Value, Scopes, depth, (symbol){}, false);
            break;
        }
        case NODE_IF: {
            // You can declare something in an if condition that is scoped only to the body of the if.
            _resolve_symbols(arena, node->If.Condition, Scopes, depth + 1, (symbol){}, false);
            _resolve_symbols(arena, node->If.ThenBlock, Scopes, depth, (symbol){}, false);
            _resolve_symbols(arena, node->If.ElseBlock, Scopes, depth, (symbol){}, false);
            break;
        }
        case NODE_WHILE: {
            _resolve_symbols(arena, node->While.Condition, Scopes, depth, (symbol){}, false);
            _resolve_symbols(arena, node->While.Body, Scopes, depth, (symbol){}, false);
            break;
        }
        case NODE_FOR: {
            _resolve_symbols(arena, node->For.Init, Scopes, depth + 1, (symbol){}, false);
            _resolve_symbols(arena, node->For.Condition, Scopes, depth + 1, (symbol){}, false);
            _resolve_symbols(arena, node->For.Advance, Scopes, depth + 1, (symbol){}, false);
            _resolve_symbols(arena, node->For.Body, Scopes, depth, (symbol){}, false);
            break;
        }
        case NODE_UNARY_OP: {
            _resolve_symbols(arena, node->UnaryOp.Operand, Scopes, depth, (symbol){}, false);
            break;
        }
        case NODE_BINARY_OP: {
            // Problem: When we are resolving fields, they are not at the
            //          current scope at all, but rather on the struct.
            //          If we're in a binary operation the symbol would
            //          already exist in some struct, along with the field
            //          offset.
            // Todo:    What we need to do is get the struct that the left
            //          is referencing, and search that struct for the field.

            // a = (entity.guy).thing + c

            symbol A = {.Type = SYM_VAR};

            symbol B = {.Type = (node->BinaryOp.Operation == TOKEN_DOT) ? SYM_FIELD : SYM_VAR};

            if (B.Type == SYM_FIELD) {
                resolve_struct_symbol_from_expr(node, Scopes, depth);
            } else {
                _resolve_symbols(arena, node->BinaryOp.Right, Scopes, depth, B, true);
                _resolve_symbols(arena, node->BinaryOp.Left, Scopes, depth, A, true);
            }

            break;
        }
        case NODE_STRUCT: {
            symbol StructSymbol = {
                .Name = node->Struct.Name->Ident.Name,
                .Type = SYM_STRUCT,
            };

            StructSymbol.Structure = make_structure_data(arena, MAX_FIELDS);

            for (int i = 0; i < node->Struct.FieldCount; i++) {
                ast_node *FieldVarDecl = node->Struct.Fields[i];

                assert(FieldVarDecl->Type == NODE_VAR_DECL);

                StructSymbol.Size += resolve_struct_decl_field(arena, Scopes, depth, &StructSymbol, FieldVarDecl);

                for (int i = 0; i < FieldVarDecl->VarDecl.ChildDeclsCount; i++) {
                    StructSymbol.Size += resolve_struct_decl_field(
                        arena, Scopes, depth, &StructSymbol, FieldVarDecl->VarDecl.ChildDecls[i]);
                }
            }

            printf("Got struct size: %d\n", StructSymbol.Size);

            resolve(node, push_symbol(arena, CurrentScope, StructSymbol));
            break;
        }
        default: {
            break;
        }
    }
}

symbols_scope make_symbols_scope(memory_arena *arena) {
    symbols_scope Scope = {};

    Scope.Symbols     = arena_push(arena, sizeof(ast_node *) * MAX_SYMBOLS);
    Scope.SymbolCount = 0;

    return Scope;
}

void resolve_symbols(ast_node *ast) {
    memory_arena arena = make_arena();

    symbols_scope *Scopes = arena_push(&arena, sizeof(symbols_scope) * MAX_DEPTH);

    for (int i = 0; i < MAX_DEPTH; i++) {
        Scopes[i] = make_symbols_scope(&arena);
    }

    _resolve_symbols(&arena, ast, Scopes, 0, (symbol){}, false);
}
