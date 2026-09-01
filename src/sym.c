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
            Error("Couldn't resolve type %.*s\n", FmtStr(Node->Struct.Fields[i]->VarDecl.Name->Ident.Name));
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

// Returns nullptr when not exist
symbol *search_for_symbol(string Name, symbols_scope *Scope) {
    symbols_scope *Curr = Scope;

    while (Curr) {
        for (int j = 0; j < Curr->SymbolCount; j++) {
            if (string_equals(Curr->Symbols[j]->Name, Name)) {
                return Curr->Symbols[j];
            }
        }

        Curr = Curr->Parent;
    }

    return nullptr;
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

symbol *resolve_struct_symbol(ast_node *StructType, symbols_scope *CurrentScope) {
    string TypeName = StructType->DataType.Name->Ident.Name;
    symbol *Sym     = search_for_symbol(TypeName, CurrentScope);

    if (Sym) {
        if (Sym->Type == SYM_STRUCT) {
            StructType->DataType.Name->Ident.Sym = Sym;
            return Sym;
        } else {
            Error("The struct type couldn't be resolved because there was another symbol with the same name.");
        }
    }

    return 0;
}

symbol *search_struct_for_field(symbol *structure, string Name) {
    for (int i = 0; i < structure->Structure.FieldCount; i++) {
        symbol *Field = structure->Structure.Fields[i];

        if (string_equals(Field->Name, Name)) return Field;
    }

    return NULL;
}

// Returns indirection depth
int resolve_type_info(memory_arena *arena, scopes *Scopes, ast_node *Type, type_info *TypeInfo) {
    assert(Type->Type == NODE_TYPE);
    type DataType = Type->DataType.Type;

    symbols_scope *CurrentScope = Scopes->CurrentScope;

    TypeInfo->Type    = DataType;
    TypeInfo->IsConst = Type->DataType.IsConst;

    if (DataType == TYPE_PTR || DataType == TYPE_ARRAY) {
        type_info *NextTypeInfo = arena_push(arena, sizeof(type_info));

        TypeInfo->IndirectionDepth = 1 + resolve_type_info(arena, Scopes, Type->DataType.PointingTo, NextTypeInfo);

        TypeInfo->IsArray = (DataType == TYPE_ARRAY);

        TypeInfo->PointingTo = NextTypeInfo;

        if (DataType == TYPE_ARRAY) {
            TypeInfo->Size = Type->DataType.ArraySize * NextTypeInfo->Size;
        } else {
            TypeInfo->Size = 8;
        }
    } else if (DataType == TYPE_STRUCT) {
        symbol *StructRef = resolve_struct_symbol(Type, CurrentScope);

        if (StructRef) {
            TypeInfo->StructType = StructRef;
            TypeInfo->Size       = StructRef->TypeInfo.Size;
        } else {
            TypeInfo->UndeclaredStructName = Type->DataType.Name->Ident.Name;
        }
    } else {
        // Primitive type, get size directly.
        TypeInfo->Size = get_type_size(DataType);
    }

    return TypeInfo->IndirectionDepth;
}

void resolve_section(symbol *Sym, ast_node *VarDecl, scopes *Scopes) {
    symbols_scope *CurrentScope = Scopes->CurrentScope;
    bool IsGlobalScope          = !CurrentScope->Parent;

    if (IsGlobalScope) {
        if (Sym->TypeInfo.IsConst) {
            Sym->Section = SECTION_RODATA;
        } else {
            bool PutInBss = !VarDecl->VarDecl.Init;

            if (!PutInBss) PutInBss |= VarDecl->VarDecl.Init->Type == NODE_INT_LIT && VarDecl->VarDecl.Init->IntegerLit.Value == 0;

            if (PutInBss) {
                Sym->Section = SECTION_BSS;
            } else {
                Sym->Section = SECTION_DATA;
            }
        }
    } else {
        if (Sym->TypeInfo.IsConst) {
            // Futrue: put it into rodata?
        }
        Sym->Section = SECTION_STACK;
    }
}

// param_index should be -1 if this variable is not a parameter
symbol make_var_decl_symbol_and_resolve_type(memory_arena *arena, scopes *Scopes, ast_node *var_decl, int param_index) {
    string Name = var_decl->VarDecl.Name->Ident.Name;

    symbol DeclSym = {
        .Name = Name,
        .Type = SYM_VAR,
    };

    if (param_index >= 0) {
        DeclSym.Section    = SECTION_REG;
        DeclSym.ParamIndex = param_index;
    }

    resolve_type_info(arena, Scopes, var_decl->VarDecl.Type, &DeclSym.TypeInfo);
    resolve_section(&DeclSym, var_decl, Scopes);

    if (var_decl->VarDecl.Type->DataType.Type == TYPE_STRUCT) {
        AssertError(DeclSym.TypeInfo.StructType != NULL, "Struct is undeclared.");
    }

    return DeclSym;
}

// param_index should be -1 if it's not a parameter. Returns new param_index
int resolve_var_decl(memory_arena *arena, scopes *Scopes, ast_node *VarDecl, int param_index) {
    symbol Sym = make_var_decl_symbol_and_resolve_type(arena, Scopes, VarDecl, param_index);

    if (param_index != -1) param_index++;

    _resolve_symbols(arena, VarDecl->VarDecl.Name, Scopes, Sym, false);
    _resolve_symbols(arena, VarDecl->VarDecl.Init, Scopes, (symbol){}, false);

    for (int i = 0; i < VarDecl->VarDecl.ChildDeclsCount; i++) {
        resolve_var_decl(arena, Scopes, VarDecl->VarDecl.ChildDecls[i], param_index);
        if (param_index != -1) param_index++;
    }

    return param_index;
}

// Returns the start of the parameter list
symbol **resolve_parameters(memory_arena *arena, ast_node *func, scopes *Scopes) {
    symbols_scope *Curr = Scopes->CurrentScope;

    symbol **Result = Curr->Symbols + Curr->SymbolCount;

    int ParamIdx = 0;

    for (int i = 0; i < func->FuncDef.ParamCount; i++) {
        ast_node *Param = func->FuncDef.Params[i];
        ParamIdx        = resolve_var_decl(arena, Scopes, Param, ParamIdx);
    }

    return Result;
}

// Returns size of all locals in the function.
int resolve_stack_offsets(ast_node *Stmt, int Offset, int *OutBlockSize) {
    if (!Stmt) return Offset;

    switch (Stmt->Type) {
        case NODE_BLOCK: {
            int SavedOffset = Offset;

            for (int i = 0; i < Stmt->Block.StatementCount; i++) {
                Offset = resolve_stack_offsets(Stmt->Block.Statements[i], Offset, OutBlockSize);
            }

            Offset = SavedOffset;
            break;
        }

        case NODE_VAR_DECL: {
            ast_node *Identifier = Stmt->VarDecl.Name;
            symbol *Sym          = Identifier->Ident.Sym;

            if (Sym->Section == SECTION_STACK) {
                Offset += Sym->TypeInfo.Size;
                Sym->StackOffset = Offset;
            }

            for (int i = 0; i < Stmt->VarDecl.ChildDeclsCount; i++) {
                Offset = resolve_stack_offsets(Stmt->VarDecl.ChildDecls[i], Offset, OutBlockSize);
            }
            break;
        }

        case NODE_FOR: {
            Offset = resolve_stack_offsets(Stmt->For.Init, Offset, OutBlockSize);
            Offset = resolve_stack_offsets(Stmt->For.Body, Offset, OutBlockSize);
            break;
        }

        case NODE_IF: {
            Offset = resolve_stack_offsets(Stmt->If.ThenBlock, Offset, OutBlockSize);
            Offset = resolve_stack_offsets(Stmt->If.ElseBlock, Offset, OutBlockSize);
            break;
        }

        case NODE_WHILE: {
            Offset = resolve_stack_offsets(Stmt->While.Body, Offset, OutBlockSize);
            break;
        }

        default: {
            break;
        }
    }

    if (Offset > *OutBlockSize) *OutBlockSize = Offset;

    return Offset;
}

// Returns the struct that this particular expression resolves to.
// It also resolves all the identifiers it sees.
// eg. entity.enemy_reference, where enemy_reference is an Enemy,
//     will return the symbol for the type of Enemy.
// eg. (entity + 1) will return symbol for type entity
// eg. *(entity + 1) will return symbol for type entity
symbol *resolve_struct_symbol_from_expr(memory_arena *arena, ast_node *node, scopes *Scopes) {
    if (node->Type == NODE_IDENT) {
        symbol *NodeSym   = search_for_symbol(node->Ident.Name, Scopes->CurrentScope);
        symbol *StructRef = NodeSym->TypeInfo.StructType;

        if (!StructRef && NodeSym->TypeInfo.PointingTo) {
            StructRef = NodeSym->TypeInfo.PointingTo->StructType;
        }

        node->Ident.Sym = NodeSym;
        return StructRef;
    } else if (node->Type == NODE_BINARY_OP) {
        if (node->BinaryOp.Operation == TOKEN_PLUS || node->BinaryOp.Operation == TOKEN_MINUS) {
            symbol *Left  = resolve_struct_symbol_from_expr(arena, node->BinaryOp.Left, Scopes);
            symbol *Right = resolve_struct_symbol_from_expr(arena, node->BinaryOp.Right, Scopes);

            if (Left) {
                return Left;
            } else if (Right) {
                return Right;
            } else {
                return NULL;
            }
        } else if (node->BinaryOp.Operation == TOKEN_OPEN_SQUARE) {
            symbol *StructRef = resolve_struct_symbol_from_expr(arena, node->BinaryOp.Left, Scopes);
            _resolve_symbols(arena, node->BinaryOp.Right, Scopes, (symbol){}, false);
            return StructRef;
        } else if (node->BinaryOp.Operation != TOKEN_DOT) {
            Error("Trying to do a binary operation that resolves to a structure? What are you doing man?!");
        } else {
            symbol *LeftStruct = resolve_struct_symbol_from_expr(arena, node->BinaryOp.Left, Scopes);

            if (node->BinaryOp.Right->Type != NODE_IDENT) {
                Error("Trying to access a struct with a token that isn't an identifier.");
            } else {
                string FieldName = node->BinaryOp.Right->Ident.Name;
                symbol *FieldSym = search_struct_for_field(LeftStruct, FieldName);

                node->BinaryOp.Right->Ident.Sym = FieldSym;

                if (!FieldSym) {
                } else {
                    type_info *TypeInfo = &FieldSym->TypeInfo;

                    while (TypeInfo->PointingTo) TypeInfo = TypeInfo->PointingTo;

                    symbol *StructRef = TypeInfo->StructType;

                    // Resolve forward declaration if StructRef is null
                    if (!StructRef && TypeInfo->UndeclaredStructName.Length > 0) {
                        string StructName = TypeInfo->UndeclaredStructName;

                        StructRef = search_for_symbol(StructName, Scopes->CurrentScope);

                        TypeInfo->StructType = StructRef;

                        if (!StructRef) {
                            Error("Couldn't find structure %.*s\n", FmtStr(StructName));
                        }
                    }

                    return StructRef;
                }
            }
        }
    } else if (node->Type == NODE_UNARY_OP) {
        // Dereference operator
        if (node->UnaryOp.Operation == TOKEN_STAR) {
            return resolve_struct_symbol_from_expr(arena, node->UnaryOp.Operand, Scopes);
        }
    }

    return NULL;
}

// Returns the size of the field
// structure is the symbol where the resulting field symbols are placed into
int resolve_struct_decl_field(memory_arena *arena, scopes *Scopes, symbol *structure, ast_node *var_decl) {
    symbol FieldSym = make_var_decl_symbol_and_resolve_type(arena, Scopes, var_decl, -1);

    FieldSym.Type        = SYM_FIELD;
    FieldSym.FieldOffset = structure->TypeInfo.Size;

    symbol *CreatedSymbol = push_field_symbol_to_struct(arena, structure, FieldSym);

    // Point the identifier to the created field symbol.
    var_decl->VarDecl.Name->Ident.Sym = CreatedSymbol;

    var_decl->Scope = Scopes->CurrentScope;

    return FieldSym.TypeInfo.Size;
}

void _resolve_symbols(memory_arena *arena, ast_node *node, scopes *Scopes, symbol current_symbol, bool must_exist) {
    if (!node) return;

    symbols_scope *CurrentScope = Scopes->CurrentScope;
    node->Scope                 = CurrentScope;

    switch (node->Type) {
        case NODE_IDENT: {
            string Name      = node->Ident.Name;
            symbol *Existing = search_for_symbol(Name, CurrentScope);

            if (must_exist && !Existing) {
                Error("Symbol should exist!");
                break;
            }

            current_symbol.Name = Name;

            resolve(node, Existing ? Existing : push_symbol(arena, CurrentScope, current_symbol));
            break;
        }
        case NODE_PROGRAM: {
            for (int i = 0; i < node->Program.GlobalDeclCount; i++)
                _resolve_symbols(arena, node->Program.GlobalDecls[i], Scopes, (symbol){}, false);

            for (int i = 0; i < node->Program.FunctionCount; i++)
                _resolve_symbols(arena, node->Program.Functions[i], Scopes, (symbol){}, false);

            break;
        }
        case NODE_TYPE: {
            if (node->DataType.Type == TYPE_STRUCT) _resolve_symbols(arena, node->DataType.Name, Scopes, current_symbol, false);
            break;
        }
        case NODE_VAR_DECL: {
            resolve_var_decl(arena, Scopes, node, -1);
            break;
        }
        case NODE_BLOCK: {
            for (int i = 0; i < node->Block.StatementCount; i++) {
                ast_node *Statement = node->Block.Statements[i];
                _resolve_symbols(arena, Statement, Scopes, (symbol){}, false);
            }
            break;
        }
        case NODE_FUNC_DEF: {
            func_data FuncData = {};

            _resolve_symbols(arena, node->FuncDef.Name, Scopes, (symbol){.Type = SYM_FUNC}, false);
            _resolve_symbols(arena, node->FuncDef.ReturnType, Scopes, (symbol){}, true);

            string FunctionName = node->FuncDef.Name->Ident.Name;

            if (Scopes->CurrentScope->Parent) {
                FuncData.LabelName        = string_make(arena, FunctionName.Length + 8);
                FuncData.LabelName.Length = sprintf(FuncData.LabelName.Data, "%.*s%d", FmtStr(FunctionName), Scopes->Count);
            } else {
                FuncData.LabelName = FunctionName;
            }

            push_scope(arena, Scopes);

            symbol **Params = resolve_parameters(arena, node, Scopes);
            _resolve_symbols(arena, node->FuncDef.Body, Scopes, (symbol){}, false);

            pop_scope(Scopes);

            FuncData.ParamCount = node->FuncDef.ParamCount;
            FuncData.Params     = Params;
            FuncData.ReturnType = node->FuncDef.ReturnType->DataType.Type;
            FuncData.IsVariadic = node->FuncDef.IsVarArg;

            symbol *Sym = node->FuncDef.Name->Ident.Sym;

            resolve_stack_offsets(node->FuncDef.Body, 0, &Sym->TypeInfo.Size);
            Sym->Function = FuncData;
            break;
        }
        case NODE_CALL: {
            symbol Sym = {.Type = SYM_FUNC};

            _resolve_symbols(arena, node->Call.FuncName, Scopes, Sym, true);

            for (int i = 0; i < node->Call.ArgCount; i++) {
                ast_node *Arg = node->Call.Args[i];

                _resolve_symbols(arena, Arg, Scopes, Sym, true);
            }
            break;
        }
        case NODE_RETURN: {
            _resolve_symbols(arena, node->Return.Value, Scopes, (symbol){}, false);
            break;
        }
        case NODE_IF: {
            _resolve_symbols(arena, node->If.Condition, Scopes, (symbol){}, false);

            push_scope(arena, Scopes);
            _resolve_symbols(arena, node->If.ThenBlock, Scopes, (symbol){}, false);
            pop_scope(Scopes);

            push_scope(arena, Scopes);
            _resolve_symbols(arena, node->If.ElseBlock, Scopes, (symbol){}, false);
            pop_scope(Scopes);
            break;
        }
        case NODE_WHILE: {
            _resolve_symbols(arena, node->While.Condition, Scopes, (symbol){}, false);
            push_scope(arena, Scopes);
            _resolve_symbols(arena, node->While.Body, Scopes, (symbol){}, false);
            pop_scope(Scopes);
            break;
        }
        case NODE_FOR: {
            push_scope(arena, Scopes);

            _resolve_symbols(arena, node->For.Init, Scopes, (symbol){}, false);
            _resolve_symbols(arena, node->For.Condition, Scopes, (symbol){}, false);
            _resolve_symbols(arena, node->For.Body, Scopes, (symbol){}, false);
            _resolve_symbols(arena, node->For.Advance, Scopes, (symbol){}, false);

            pop_scope(Scopes);
            break;
        }
        case NODE_UNARY_OP: {
            _resolve_symbols(arena, node->UnaryOp.Operand, Scopes, (symbol){}, false);
            break;
        }
        case NODE_BINARY_OP: {
            symbol A = {.Type = SYM_VAR};
            symbol B = {.Type = (node->BinaryOp.Operation == TOKEN_DOT) ? SYM_FIELD : SYM_VAR};

            if (B.Type == SYM_FIELD) {
                resolve_struct_symbol_from_expr(arena, node, Scopes);
            } else {
                _resolve_symbols(arena, node->BinaryOp.Right, Scopes, B, true);
                _resolve_symbols(arena, node->BinaryOp.Left, Scopes, A, true);
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

                StructSymbol.TypeInfo.Size += resolve_struct_decl_field(arena, Scopes, &StructSymbol, FieldVarDecl);

                for (int i = 0; i < FieldVarDecl->VarDecl.ChildDeclsCount; i++) {
                    StructSymbol.TypeInfo.Size +=
                        resolve_struct_decl_field(arena, Scopes, &StructSymbol, FieldVarDecl->VarDecl.ChildDecls[i]);
                }
            }

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

symbols_scope *push_scope(memory_arena *Arena, scopes *Scopes) {
    Scopes->Scopes[Scopes->Count] = make_symbols_scope(Arena);

    symbols_scope *ParentScope = Scopes->CurrentScope;

    Scopes->CurrentScope         = Scopes->Scopes + (Scopes->Count++);
    Scopes->CurrentScope->Parent = ParentScope;

    return Scopes->CurrentScope;
}

void pop_scope(scopes *Scopes) { Scopes->CurrentScope = Scopes->CurrentScope->Parent; }

void resolve_symbols(ast_node *ast) {
    memory_arena arena = make_arena();

    scopes Scopes;

    Scopes.Scopes = arena_push(&arena, sizeof(symbols_scope *) * MAX_SCOPES);
    Scopes.Count  = 0;

    // Push global scope.
    push_scope(&arena, &Scopes);

    _resolve_symbols(&arena, ast, &Scopes, (symbol){}, false);
}
