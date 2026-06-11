#include "parser.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "parser_expressions.c"

ast_node *node(parser *p, node_type type) {
    ast_node *Node = arena_push(p->Arena, sizeof(ast_node));

    *Node = (ast_node){};

    if (!Node) {
        printf("Allocation error.\n");
        exit(1);
    }

    Node->Type = type;

    return Node;
}

bool has_next(parser *p) { return p->i < p->Tokens.Count; }

void parse_error(parser *p, const char *format, ...) {
    va_list Args;
    va_start(Args, format);

    printf("[Parser Error] ");

    vprintf(format, Args);

    printf("\n");

    print_at(p);

    va_end(Args);
}

token *advance(parser *p) {
    if (has_next(p)) return &p->Tokens.Tokens[p->i++];

    return NULL;
}

token *advance_count(parser *p, int offset) {
    if (p->i + offset >= p->Tokens.Count || p->i + offset < 0) return NULL;

    p->i += offset;
    return &p->Tokens.Tokens[p->i];
}

token *back(parser *p) {
    if (p->i > 0) return &p->Tokens.Tokens[p->i--];

    return NULL;
}

token *peek(parser *p) { return &p->Tokens.Tokens[p->i]; }

token *peek_next(parser *p, int offset) {
    if (p->i + offset > p->Tokens.Count || p->i + offset < 0) return NULL;

    return &p->Tokens.Tokens[p->i + offset];
}

bool expect(parser *p, token_type expected) { return (peek(p)->Type == expected); }

bool expect_tokens(parser *p, int count, ...) {
    va_list Args;

    va_start(Args, count);

    for (int i = 0; i < count; i++) {
        token_type Arg = va_arg(Args, token_type);

        if (peek_next(p, i) && peek_next(p, i)->Type != Arg) return false;
    }

    va_end(Args);

    return true;
}

bool expect_keyword(parser *p, keyword k) {
    return (peek(p)->Type == TOKEN_KEYWORD && peek(p)->Keyword == k);
}

token *_consume(parser *p, token_type expected, const char *_file, int _line) {
    token *Tok = peek(p);
    if (Tok->Type != expected) {
        parse_error(p, "%s(%d) Expected %s but got %s\n", _file, _line, token_name(expected),
                    token_name(Tok->Type));
        raise(SIGTRAP);
    }
    p->i++;
    return Tok;
}

token *consume_keyword(parser *p, keyword Keyword) {
    token *Tok = peek(p);

    if (Tok->Type != TOKEN_KEYWORD || Tok->Keyword != Keyword) {
        parse_error(p, "Expected keyword %s, got %s\n", get_keyword_str(Keyword),
                    get_keyword_str(Tok->Keyword));
    }
    p->i++;
    return Tok;
}

ast_node *parse_identifier(parser *p) {
    token *Ident     = advance(p);
    ast_node *Node   = node(p, NODE_IDENT);
    Node->Ident.Name = Ident->String;
    return Node;
}

ast_node *parse_literal(parser *p) {
    token *Lit = advance(p);

    ast_node *Node = 0;

    if (Lit->Type == TOKEN_QUOTE || Lit->Type == TOKEN_CHAR_QUOTE) {
        Lit = advance(p);
    }

    // TODO: No floating point yet...
    switch (Lit->Type) {
        case TOKEN_NUMBER:
            Node                   = node(p, NODE_INT_LIT);
            Node->IntegerLit.Value = atoll(string_to_cstr(Lit->String));
            break;
        case TOKEN_STRING_LIT:
            Node                  = node(p, NODE_STRING_LIT);
            Node->StringLit.Value = Lit->String;
            consume(p, TOKEN_QUOTE);
            break;
        case TOKEN_CHAR_LIT:
            Node                = node(p, NODE_CHAR_LIT);
            Node->CharLit.Value = Lit->String.Data[0];
            consume(p, TOKEN_CHAR_QUOTE);
            break;
        default: parse_error(p, "Couldn't parse literal."); break;
    }

    return Node;
}

ast_node *parse_function_call(parser *p) {
    ast_node *Node = node(p, NODE_CALL);

    Node->Call.FuncName = advance(p)->String;

    consume(p, TOKEN_OPEN_PAREN);

    if (peek(p)->Type == TOKEN_CLOSE_PAREN) {
        // No arguments

        Node->Call.ArgCount = 0;
        Node->Call.Args     = 0;
    } else {
        // We have at least 1 argument

        int Saved = p->i;  // Save pointer

        int ArgCount = 1;

        while (has_next(p) && peek(p)->Type != TOKEN_CLOSE_PAREN) {
            if (advance(p)->Type == TOKEN_COMMA) ArgCount++;
        }

        p->i = Saved;  // Restore pointer

        // Allocate args
        Node->Call.ArgCount = ArgCount;
        Node->Call.Args     = arena_push(p->Arena, ArgCount * sizeof(ast_node *));
        int i               = 0;

        while (has_next(p) && peek(p)->Type != TOKEN_CLOSE_PAREN) {
            ast_node *Expr = parse_expression(p);

            if (Expr) {
                Node->Call.Args[i++] = Expr;
            }

            if (peek(p)->Type == TOKEN_COMMA) advance(p);
        }
    }

    consume(p, TOKEN_CLOSE_PAREN);

    return Node;
}

bool is_function_call(parser *p) { return expect_tokens(p, 2, TOKEN_IDENTIFIER, TOKEN_OPEN_PAREN); }

/*
bool is_expression(parser *p) {
    return expect(p, TOKEN_IDENTIFIER) || expect(p, TOKEN_NUMBER) || expect(p, TOKEN_QUOTE);
}
*/

ast_node *parse_return(parser *p) {
    ast_node *Node = 0;

    consume(p, TOKEN_KEYWORD);

    ast_node *ValNode = parse_expression(p);

    if (!ValNode) {
        parse_error(p, "Error reading return value in return statement.");
        return Node;
    }

    Node               = node(p, NODE_RETURN);
    Node->Return.Value = ValNode;

    return Node;
}

bool is_return(parser *p) { return expect(p, TOKEN_KEYWORD) && peek(p)->Keyword == KEYWORD_RETURN; }

type get_data_type(string s) {
    if (string_equals(s, CSTR("int")))
        return TYPE_S16;
    else if (string_equals(s, CSTR("int8")))
        return TYPE_S8;
    else if (string_equals(s, CSTR("int16")))
        return TYPE_S16;
    else if (string_equals(s, CSTR("int32")))
        return TYPE_S32;
    else if (string_equals(s, CSTR("uint")))
        return TYPE_U16;
    else if (string_equals(s, CSTR("uint8")))
        return TYPE_U8;
    else if (string_equals(s, CSTR("uint16")))
        return TYPE_U16;
    else if (string_equals(s, CSTR("uint32")))
        return TYPE_U32;
    else if (string_equals(s, CSTR("float")))
        return TYPE_FLOAT;
    else if (string_equals(s, CSTR("void")))
        return TYPE_VOID;
    else
        return TYPE_STRUCT;
}

string get_type_name(type t) {
    switch (t) {
        case TYPE_S8:     return CSTR("int8");
        case TYPE_S16:    return CSTR("int16");
        case TYPE_S32:    return CSTR("int32");
        case TYPE_U8:     return CSTR("uint8");
        case TYPE_U16:    return CSTR("uint16");
        case TYPE_U32:    return CSTR("uint32");
        case TYPE_ARRAY:  return CSTR("array");
        case TYPE_PTR:    return CSTR("pointer");
        case TYPE_VOID:   return CSTR("void");
        case TYPE_FLOAT:  return CSTR("float");
        case TYPE_STRUCT: return CSTR("struct");
    }

    return (string){};
}

ast_node *_parse_type(parser *p, bool peeking) {
    ast_node *BaseNode = 0;
    ast_node *Prev     = 0;

    while (peek(p)->Type == TOKEN_OPEN_SQUARE || peek(p)->Type == TOKEN_STAR) {
        ast_node *New = 0;

        if (!peeking) New = node(p, NODE_TYPE);

        if (peek(p)->Type == TOKEN_OPEN_SQUARE) {
            if (peeking) {
                advance_count(p, 3);
                continue;
            }

            New->DataType.Type = TYPE_ARRAY;

            advance(p);
            New->DataType.ArraySize = string_to_ll(advance(p)->String);
        } else {
            if (peeking) {
                advance(p);
                continue;
            }

            New->DataType.Type = TYPE_PTR;
        }

        if (Prev) Prev->DataType.PointingTo = New;

        if (!BaseNode) BaseNode = New;

        Prev = New;

        advance(p);
    }

    if (peeking) {
        advance(p);
        return NULL;
    }

    if (peek(p)->Type == TOKEN_IDENTIFIER) {
        ast_node *EndNode         = node(p, NODE_TYPE);
        EndNode->DataType.Name    = advance(p)->String;
        EndNode->DataType.Type    = get_data_type(EndNode->DataType.Name);
        if (Prev) Prev->DataType.PointingTo = EndNode;

        return BaseNode ? BaseNode : EndNode;
    } else {
        parse_error(p, "Expected an identifier for the type, but got %s\n",
                    token_name(peek(p)->Type));
        return NULL;
    }
}

ast_node *parse_type(parser *p) { return _parse_type(p, false); }

token *advance_type(parser *p) {
    _parse_type(p, true);
    return peek(p);
}

bool is_type(parser *p) {
    return expect(p, TOKEN_STAR) || expect(p, TOKEN_OPEN_SQUARE) || expect(p, TOKEN_IDENTIFIER);
}

ast_node *parse_var_decl(parser *p) {
    ast_node *Type = parse_type(p);
    token *VarName = consume(p, TOKEN_IDENTIFIER);

    ast_node *RhsNode = 0;

    if (peek(p)->Type == TOKEN_EQUALS) {
        consume(p, TOKEN_EQUALS);
        RhsNode = parse_expression(p);
    }

    ast_node *Node     = node(p, NODE_VAR_DECL);
    Node->VarDecl.Type = Type;
    Node->VarDecl.Name = VarName->String;
    Node->VarDecl.Init = RhsNode;

    return Node;
}

bool is_var_decl(parser *p) { return is_type(p); }

ast_node *parse_block(parser *p) {
    ast_node *Block = node(p, NODE_BLOCK);

    Block->Block.Statements = arena_push(p->Arena, MAX_STATEMENTS * sizeof(ast_node *));

    bool Single = peek(p)->Type != TOKEN_OPEN_SCOPE;

    if (!Single) consume(p, TOKEN_OPEN_SCOPE);

    while (has_next(p) && peek(p)->Type != TOKEN_CLOSE_SCOPE) {
        node_type StatementType = next_statement_type(p);

        ast_node *Expr = parse_statement(p, StatementType);

        Block->Block.Statements[Block->Block.StatementCount++] = Expr;

        if (Single) break;
    }

    if (!Single) consume(p, TOKEN_CLOSE_SCOPE);

    return Block;
}

bool is_block(parser *p) { return expect(p, TOKEN_OPEN_SCOPE); }

ast_node *parse_if(parser *p) {
    consume(p, TOKEN_KEYWORD);
    ast_node *Condition = 0, *Body = 0, *Else = 0, *Node = 0;

    Condition = parse_expression(p);

    if (is_block(p)) {
        Body = parse_block(p);
    } else {
        node_type StmtType = next_statement_type(p);

        if (StmtType)
            Body = parse_statement(p, StmtType);
        else
            parse_error(p, "Couldn't read statement after if statement");
    }

    if (expect_keyword(p, KEYWORD_ELSE)) {
        advance(p);

        if (is_block(p)) {
            Else = parse_block(p);
        } else {
            node_type StmtType = next_statement_type(p);

            if (StmtType)
                Else = parse_statement(p, StmtType);
            else
                parse_error(p, "Couldn't read statement after if statement");
        }
    }

    Node               = node(p, NODE_IF);
    Node->If.Condition = Condition;
    Node->If.ThenBlock = Body;
    Node->If.ElseBlock = Else;

    return Node;
}

bool is_if(parser *p) { return expect_keyword(p, KEYWORD_IF); }

bool is_for(parser *p) { return expect_keyword(p, KEYWORD_FOR); }

ast_node *parse_for(parser *p) {
    ast_node *Node = node(p, NODE_FOR);

    consume_keyword(p, KEYWORD_FOR);

    if (peek(p)->Type == TOKEN_OPEN_PAREN) consume(p, TOKEN_OPEN_PAREN);

    // Init
    if (peek(p)->Type == TOKEN_END_STATEMENT) {
        advance(p);  // left blank
    } else {
        if (is_var_decl(p)) {
            Node->For.Init = parse_var_decl(p);
        } else {
            Node->For.Init = parse_expression(p);
        }
        consume(p, TOKEN_END_STATEMENT);
    }

    // Condition
    if (peek(p)->Type == TOKEN_END_STATEMENT) {
        advance(p);  // left blank
    } else {
        Node->For.Condition = parse_expression(p);
        consume(p, TOKEN_END_STATEMENT);
    }

    // Advance
    if (peek(p)->Type == TOKEN_CLOSE_PAREN) {
    } else {
        Node->For.Advance = parse_expression(p);
    }

    advance(p);

    Node->For.Body = parse_block(p);

    return Node;
}

bool is_while(parser *p) { return expect_keyword(p, KEYWORD_WHILE); }

ast_node *parse_while(parser *p) {
    ast_node *Node = node(p, NODE_WHILE);

    consume_keyword(p, KEYWORD_WHILE);

    Node->While.Condition = parse_expression(p);
    Node->While.Body      = parse_block(p);

    return Node;
}

node_type next_statement_type(parser *p) {
    if (is_function(p)) return NODE_FUNC_DEF;
    if (is_function_call(p)) return NODE_CALL;
    if (is_block(p)) return NODE_BLOCK;
    if (is_return(p)) return NODE_RETURN;
    if (is_var_decl(p)) return NODE_VAR_DECL;
    if (is_if(p)) return NODE_IF;
    if (is_for(p)) return NODE_FOR;
    if (is_while(p)) return NODE_WHILE;

    // Not a statement OR couldn't determine what type of statement was next.

    return NODE_INVALID;
}

ast_node *parse_statement(parser *p, node_type type) {
    ast_node *Node = 0;

    bool ExpectSemicolon = false;

    switch (type) {
        case NODE_FUNC_DEF: Node = parse_function(p); break;
        case NODE_CALL:
            Node = parse_function_call(p);

            ExpectSemicolon = true;
            break;
        case NODE_BLOCK: Node = parse_block(p); break;
        case NODE_RETURN:
            Node = parse_return(p);

            ExpectSemicolon = true;
            break;
        case NODE_VAR_DECL:
            Node = parse_var_decl(p);

            ExpectSemicolon = true;
            break;
        case NODE_IF:    Node = parse_if(p); break;
        case NODE_FOR:   Node = parse_for(p); break;
        case NODE_WHILE: Node = parse_while(p); break;
        default:
            Node            = parse_expression(p);
            ExpectSemicolon = true;
            break;
    }

    if (ExpectSemicolon) consume(p, TOKEN_END_STATEMENT);

    return Node;
}

ast_node **get_params(parser *p, int *param_count) {
    ast_node **Result = 0;

    // Get parameter count
    int ParamCount = 0;

    {
        int StoredI = p->i;

        consume(p, TOKEN_OPEN_PAREN);

        while (peek(p)->Type != TOKEN_CLOSE_PAREN) {
            ParamCount++;

            advance_type(p);
            consume(p, TOKEN_IDENTIFIER);

            if (peek(p)->Type == TOKEN_COMMA) consume(p, TOKEN_COMMA);
        }

        if (param_count) *param_count = ParamCount;

        p->i = StoredI;
    }

    // Allocate array of parameters
    Result = arena_push(p->Arena, ParamCount * sizeof(ast_node *));
    int i  = 0;

    // Advance forward, and create new ast_node*, filling them into the Results array
    consume(p, TOKEN_OPEN_PAREN);

    while (peek(p)->Type != TOKEN_CLOSE_PAREN) {
        ast_node *ParamType = parse_type(p);
        token *ParamName    = consume(p, TOKEN_IDENTIFIER);

        ast_node *Param = node(p, NODE_VAR_DECL);

        Param->VarDecl.Init = 0;
        Param->VarDecl.Name = ParamName->String;
        Param->VarDecl.Type = ParamType;

        Result[i++] = Param;

        if (peek(p)->Type == TOKEN_COMMA) consume(p, TOKEN_COMMA);
    }

    consume(p, TOKEN_CLOSE_PAREN);

    return Result;
}

void move_to_end_of_block(parser *p) {
    assert(peek(p)->Type == TOKEN_OPEN_SCOPE);

    int Balance = 0;
    token *T    = 0;

    while ((T = advance(p))) {
        if (T->Type == TOKEN_OPEN_SCOPE)
            Balance++;
        else if (T->Type == TOKEN_CLOSE_SCOPE)
            Balance--;

        if (Balance <= 0) break;
    }

    back(p);
}

bool is_function(parser *p) {
    if (!is_type(p)) return false;

    int Saved = p->i;
    advance_type(p);
    bool Result = peek_next(p, 0)->Type == TOKEN_IDENTIFIER && peek_next(p, 1)->Type == TOKEN_OPEN_PAREN;

    p->i = Saved;
    return Result;
}

ast_node *parse_function(parser *p) {
    int ParamCount = 0;

    ast_node *ReturnType = parse_type(p);
    token *FuncName   = consume(p, TOKEN_IDENTIFIER);

    ast_node **Params = get_params(p, &ParamCount);
    ast_node *Body    = parse_block(p);
    ast_node *Root    = node(p, NODE_FUNC_DEF);

    Root->FuncDef.Name       = FuncName->String;
    Root->FuncDef.ReturnType = ReturnType;
    Root->FuncDef.Params     = Params;
    Root->FuncDef.ParamCount = ParamCount;
    Root->FuncDef.Body       = Body;

    return Root;
}

int get_top_level_function_count(parser *p) {
    int Count = 0;
    int Saved = p->i;

    do {
        if (is_function(p)) {
            Count++;
            while (has_next(p) && advance(p)->Type != TOKEN_OPEN_SCOPE);
            back(p);
            move_to_end_of_block(p);
        } else {
            advance(p);
        }
    } while (has_next(p));

    p->i = Saved;

    return Count;
}

ast_node *parse(memory_arena *arena, token_list tokens) {
    parser p = {0, 0, tokens, arena};

    ast_node *Root = node(&p, NODE_PROGRAM);

    Root->Program.FunctionCount = get_top_level_function_count(&p);
    Root->Program.Functions  = arena_push(arena, Root->Program.FunctionCount * sizeof(ast_node *));
    Root->Program.GlobalVars = 0;

    int i = 0;

    while (has_next(&p)) {
        node_type StatementType = next_statement_type(&p);

        if (StatementType == NODE_INVALID) break;

        ast_node *Statement = parse_statement(&p, StatementType);

        if (Statement) {
            if (Statement->Type == NODE_FUNC_DEF)
                Root->Program.Functions[i++] = Statement;
            else if (Statement->Type == NODE_VAR_DECL)
                Root->Program.GlobalVars[Root->Program.GlobalVarCount++] = Statement;
        }
    }

    return Root;
}

void print_node_type(ast_node *node) {
    printf(ANSI_DIM);

    switch (node->Type) {
        case NODE_PROGRAM:    printf("[Program Root]"); break;
        case NODE_FUNC_DEF:   printf("[Function]"); break;
        case NODE_BLOCK:      printf("[Block]"); break;
        case NODE_VAR_DECL:   printf("[Declaration]"); break;
        case NODE_ASSIGN:     printf("[Assignment]"); break;
        case NODE_RETURN:     printf("[Return]"); break;
        case NODE_IF:         printf("[If]"); break;
        case NODE_WHILE:      printf("[While]"); break;
        case NODE_FOR:        printf("[For]"); break;
        case NODE_CALL:       printf("[Call]"); break;
        case NODE_BINARY_OP:  printf("[Binary Op]"); break;
        case NODE_UNARY_OP:   printf("[Unary Op]"); break;
        case NODE_IDENT:      printf("[Identifier]"); break;
        case NODE_INT_LIT:    printf("[Integer Lit]"); break;
        case NODE_FLOAT_LIT:  printf("[Float Lit]"); break;
        case NODE_STRING_LIT: printf("[String Lit]"); break;
        case NODE_CHAR_LIT:   printf("[Char Lit]"); break;
        default:              printf("[Unknown Node]"); break;
    }

    printf(ANSI_RESET);
}

void print_type(ast_node *Type) {
    switch (Type->DataType.Type) {
        case TYPE_ARRAY: printf("[%d]", Type->DataType.ArraySize); break;
        case TYPE_PTR:   printf("*"); break;
        default:         string_print_b2(Type->DataType.Name);
    }

    if (Type->DataType.PointingTo) print_type(Type->DataType.PointingTo);
}

void print_function_node(ast_node *node) {
    if (node->Type != NODE_FUNC_DEF) return;

    string_print_b3(node->FuncDef.Name);

    printf(ANSI_DIM);
    printf(" returns: ");
    printf(ANSI_RESET);

    print_type(node->FuncDef.ReturnType);

    printf(ANSI_DIM);
    printf(", params: ");
    printf(ANSI_RESET);

    for (int i = 0; i < node->FuncDef.ParamCount; i++) {
        ast_node *Type = node->FuncDef.Params[i]->VarDecl.Type;
        print_type(Type);

        printf(" ");
        string_print_b(node->FuncDef.Params[i]->VarDecl.Name);

        if (i < node->FuncDef.ParamCount - 1) {
            printf(", ");
        }
    }
}

void print_node(ast_node *node) {
    print_node_type(node);
    printf(" ");

    switch (node->Type) {
        case NODE_PROGRAM:  break;
        case NODE_FUNC_DEF: print_function_node(node); break;
        case NODE_BLOCK:    break;
        case NODE_IF:
            printf(ANSI_DIM "Condition: " ANSI_RESET);
            print_node(node->If.Condition);
            break;
        case NODE_WHILE:
            printf(ANSI_DIM "Condition: " ANSI_RESET);
            print_node(node->While.Condition);
            break;
        case NODE_FOR:
            printf(ANSI_DIM "{ Init: " ANSI_RESET);
            if (node->For.Init)
                print_node(node->For.Init);
            else
                printf(ANSI_DIM "(none)" ANSI_RESET);
            printf(ANSI_DIM ", Condition: " ANSI_RESET);
            if (node->For.Condition)
                print_node(node->For.Condition);
            else
                printf(ANSI_DIM "(none)" ANSI_RESET);
            printf(ANSI_DIM ", Advance: " ANSI_RESET);
            if (node->For.Advance)
                print_node(node->For.Advance);
            else
                printf(ANSI_DIM "(none)" ANSI_RESET);
            printf(ANSI_DIM " }" ANSI_RESET);
            break;
        case NODE_RETURN: print_node(node->Return.Value); break;
        case NODE_ASSIGN:
            string_print_b(node->Assign.Target);
            printf(ANSI_DIM " = " ANSI_RESET);
            print_node(node->Assign.Value);
            break;
        case NODE_VAR_DECL:
            print_type(node->VarDecl.Type);
            printf(" ");
            string_print_b(node->VarDecl.Name);
            if (node->VarDecl.Init) {
                printf(ANSI_DIM " = " ANSI_RESET);
                print_node(node->VarDecl.Init);
            }
            break;
        case NODE_INT_LIT:
            printf(ANSI_FG_MAGENTA);
            printf("%lld", node->IntegerLit.Value);
            printf(ANSI_RESET);
            return;
        case NODE_STRING_LIT:
            printf("\"");
            string_print_b4(node->StringLit.Value);
            printf("\"");
            return;
        case NODE_CHAR_LIT:
            printf("'");
            string_print_b4((string){&node->CharLit.Value, 1});
            printf("'");
            return;
        case NODE_IDENT: string_print_b(node->Ident.Name); return;
        case NODE_CALL:
            string_print_b3(node->Call.FuncName);

            printf(ANSI_DIM " params: " ANSI_RESET);

            for (int i = 0; i < node->Call.ArgCount; i++) {
                print_node(node->Call.Args[i]);

                if (i < node->Call.ArgCount - 1) printf(", ");
            }
            break;
        case NODE_BINARY_OP:
            printf(ANSI_DIM "{ Op: " ANSI_RESET);
            printf("%s", token_name(node->BinaryOp.Operation));
            printf(ANSI_DIM ", Left: " ANSI_RESET);
            print_node(node->BinaryOp.Left);
            printf(ANSI_DIM ", Right: " ANSI_RESET);
            print_node(node->BinaryOp.Right);
            printf(ANSI_DIM " }" ANSI_RESET);
            break;
        case NODE_UNARY_OP:
            printf(ANSI_DIM "{ Op: " ANSI_RESET "%s, " ANSI_DIM "Operand: " ANSI_RESET,
                   token_name(node->UnaryOp.Operation));
            print_node(node->UnaryOp.Operand);
            printf(ANSI_DIM ", First: " ANSI_FG_MAGENTA "%s" ANSI_RESET,
                   node->UnaryOp.First ? "true" : "false");
            printf(ANSI_DIM " }" ANSI_RESET);
            break;
        default: printf("(didn't implement printing for node type %d yet!)", node->Type); break;
    }
}

void print_at(parser *p) {
    printf("==(Debug Trace)==\n");

    for (int i = -3; i <= 3; i++) {
        int idx = p->i + i;

        if (idx < 0 || idx >= p->Tokens.Count) continue;

        token *T = &p->Tokens.Tokens[idx];

        printf("%4d", idx);

        if (i == 0)
            printf(" >");
        else
            printf("  ");

        print_token(T);
        printf("\n");
    }
}

void _print_tree(ast_node *ast, const char *message, int depth, int *last_child_flags) {
    printf(FG_COLOR(236));

    for (int i = 0; i < depth - 1; i++) {
        printf(last_child_flags[i] ? "    " : "│   ");
    }
    if (depth > 0) {
        printf(last_child_flags[depth - 1] ? "└── " : "├── ");
    }

    printf(ANSI_RESET);
    if (message) printf(FG_COLOR(247) "(%s) " ANSI_RESET, message);
    print_node(ast);
    printf("\n");

    int new_flags[64];
    if (depth > 0) memcpy(new_flags, last_child_flags, depth * sizeof(int));

    switch (ast->Type) {
        case NODE_PROGRAM: {
            assert(depth == 0);
            int total = ast->Program.FunctionCount + ast->Program.GlobalVarCount;
            int idx   = 0;

            for (int i = 0; i < ast->Program.FunctionCount; i++, idx++) {
                new_flags[depth] = (idx == total - 1);
                _print_tree(ast->Program.Functions[i], 0, depth + 1, new_flags);
            }

            for (int i = 0; i < ast->Program.GlobalVarCount; i++, idx++) {
                new_flags[depth] = (idx == total - 1);
                _print_tree(ast->Program.GlobalVars[i], 0, depth + 1, new_flags);
            }
            break;
        }
        case NODE_FUNC_DEF:
            new_flags[depth] = 1;
            _print_tree(ast->FuncDef.Body, 0, depth + 1, new_flags);
            break;
        case NODE_BLOCK:
            for (int i = 0; i < ast->Block.StatementCount; i++) {
                new_flags[depth] = (i == ast->Block.StatementCount - 1);
                _print_tree(ast->Block.Statements[i], 0, depth + 1, new_flags);
            }
            break;
        case NODE_IF:
            _print_tree(ast->If.ThenBlock, "Then", depth + 1, new_flags);

            if (ast->If.ElseBlock) {
                _print_tree(ast->If.ElseBlock, "Else", depth + 1, new_flags);
            }
            break;
        case NODE_WHILE: _print_tree(ast->While.Body, 0, depth + 1, new_flags); break;
        case NODE_FOR:   _print_tree(ast->For.Body, 0, depth + 1, new_flags); break;
        default:         break;
    }
}

void print_tree(ast_node *ast) {
    int flags[64] = {0};
    _print_tree(ast, 0, 0, flags);
}
