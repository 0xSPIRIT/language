#include "parser.h"

#include <stdarg.h>

#define consume(p, token) _consume(p, token, __FILE__, __LINE__)

ast_node *node(parser *p, node_type type) {
    ast_node *Node = arena_push(p->Arena, sizeof(ast_node));

    Node->Type = type;

    if (!Node) {
        printf("Allocation error.\n");
        exit(1);
    }

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
    // print_stack_trace();

    va_end(Args);
}

token *advance(parser *p) {
    if (has_next(p)) return &p->Tokens.Tokens[p->i++];

    return NULL;
}

token *advance_count(parser *p, int offset) {
    if (p->i + offset > p->Tokens.Count || p->i + offset < 0) return NULL;

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

token *_consume(parser *p, token_type expected, const char *file, int line) {
    token *Tok = peek(p);
    if (Tok->Type != expected) {
        parse_error(p, "%s(%d) Expected %s but got %s\n", file, line, token_name(expected),
                    token_name(Tok->Type));
    }
    p->i++;
    return Tok;
}

token *consume_keyword(parser *p, keyword Keyword) {
    token *Tok = peek(p);

    if (Tok->Type != TOKEN_KEYWORD && Tok->Keyword != Keyword) {
        parse_error(p, "Expected keyword %s, got %s\n", get_keyword_str(Keyword),
                    get_keyword_str(Tok->Keyword));
    }
    p->i++;
    return Tok;
}

ast_node *parse_assignment(parser *p) {
    ast_node *Root = node(p, NODE_ASSIGN);

    token *Left = consume(p, TOKEN_IDENTIFIER);
    (void)Left;
    consume(p, TOKEN_EQUALS);

    return Root;
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

    // TODO: No floating point yet...
    switch (Lit->Type) {
        case TOKEN_NUMBER:
            Node                   = node(p, NODE_INT_LIT);
            Node->IntegerLit.Value = atoll(string_to_cstr(Lit->String));
            break;
        case TOKEN_STRING_LIT:
            Node                  = node(p, NODE_STRING_LIT);
            Node->StringLit.Value = Lit->String;
            break;
        default:
            parse_error(p, "Couldn't parse literal.");
            break;
    }

    return Node;
}

// if peeking, don't generate any parser errors,
// and we will return non-NULL if the following tokens
// are some kind of statement.
//
// else, we create the node and return it.
ast_node *_parse_or_peek_statement(parser *p, bool peeking) {
    int Saved = p->i;

    ast_node *Node = 0;

    if (peeking) {
        // If peeking, we just want to return a non-zero value;
        Node = (ast_node *)(-1);
    }

    // First
    token *Curr = advance(p);

    // parse return OR
    // parse assignment OR
    // parse call OR
    // parse if OR
    // parse while OR
    // parse return OR
    // parse block

    switch (Curr->Type) {
        case TOKEN_KEYWORD:
            if (Curr->Keyword == KEYWORD_RETURN) {
                token *RetVal = peek(p);

                if (RetVal->Type == TOKEN_IDENTIFIER) {
                    advance(p);

                    if (peek(p)->Type != TOKEN_END_STATEMENT) {
                        parse_error(p, "Expected ; at end of statement.");
                        Node = 0;
                        break;
                    }

                    back(p);

                    if (peeking) break;

                    ast_node *Identifier = parse_identifier(p);

                    Node               = node(p, NODE_RETURN);
                    Node->Return.Value = Identifier;
                } else if (RetVal->Type == TOKEN_NUMBER || RetVal->Type == TOKEN_STRING_LIT) {
                    advance(p);

                    if (peek(p)->Type != TOKEN_END_STATEMENT) {
                        parse_error(p, "Expected ; at end of statement.");
                        Node = 0;
                        break;
                    }

                    back(p);

                    if (peeking) break;

                    ast_node *Literal = parse_literal(p);

                    Node               = node(p, NODE_RETURN);
                    Node->Return.Value = Literal;
                } else {
                    parse_error(p, "Expected identifier after return, but got ");
                    string_print(peek(p)->String);
                    Node = 0;
                }
            }
            break;
        case TOKEN_IDENTIFIER: {
            // Second
            token *T = advance(p);

            if (T->Type == TOKEN_OPEN_SCOPE) {
                // Function call
            } else if (T->Type == TOKEN_IDENTIFIER) {
                // Variable declaration (int x = 5)
                ast_node *RhsNode = 0;

                consume(p, TOKEN_EQUALS);

                // TODO: Parse expression rather than just one.
                if (peek(p)->Type == TOKEN_IDENTIFIER) {
                    if (peeking) break;

                    RhsNode = parse_assignment(p);
                } else if (peek(p)->Type == TOKEN_NUMBER || peek(p)->Type == TOKEN_STRING_LIT) {
                    if (peeking) break;

                    RhsNode = parse_literal(p);
                } else {
                    parse_error(p, "Expected identifier or string/integer literal but got ");
                    string_print(peek(p)->String);
                    break;
                }

                Node                   = node(p, NODE_VAR_DECL);
                Node->VarDecl.TypeName = Curr->String;
                Node->VarDecl.Name     = T->String;
                Node->VarDecl.Init     = RhsNode;
            } else if (T->Type == TOKEN_EQUALS) {
                token *Rhs = advance(p);

                if (peek(p)->Type != TOKEN_END_STATEMENT) {
                    parse_error(p, "Expected ; at end of statement.");
                    Node = 0;
                    break;
                }

                if (Rhs->Type == TOKEN_IDENTIFIER) {
                    if (peeking) break;

                    ast_node *Ident   = node(p, NODE_IDENT);
                    Ident->Ident.Name = Rhs->String;

                    Node                = node(p, NODE_ASSIGN);
                    Node->Assign.Target = Curr->String;
                    Node->Assign.Value  = Ident;
                } else if (Rhs->Type == TOKEN_NUMBER || Rhs->Type == TOKEN_STRING_LIT) {
                    if (peeking) break;

                    back(p);
                    ast_node *Value = parse_literal(p);

                    Node                = node(p, NODE_ASSIGN);
                    Node->Assign.Target = Curr->String;
                    Node->Assign.Value  = Value;
                } else {
                    parse_error(p, "Expected identifier or string/integer literal but got ");
                    string_print(peek(p)->String);
                }
            }
            break;
        }
        default:
            p->i = Saved;
            return NULL;
    }

    if (!peeking) {
        consume(p, TOKEN_END_STATEMENT);
    } else {
        p->i = Saved;
    }

    return Node;
}

ast_node *parse_statement(parser *p) { return _parse_or_peek_statement(p, false); }

bool is_statement(parser *p) { return _parse_or_peek_statement(p, true) != 0; }

ast_node *parse_block(parser *p) {
    ast_node *Block = node(p, NODE_BLOCK);

    Block->Block.Statements = arena_push(p->Arena, MAX_STATEMENTS * sizeof(ast_node *));

    consume(p, TOKEN_OPEN_SCOPE);

    int Balance = 1;

    while (true) {
        if (is_statement(p)) {
            ast_node *Expr = parse_statement(p);
            Block->Block.Statements[Block->Block.StatementCount++] = Expr;
        } else {
            token *Tok = advance(p);

            if (Tok) {
                if (Tok->Type == TOKEN_OPEN_SCOPE)
                    Balance++;
                else if (Tok->Type == TOKEN_CLOSE_SCOPE)
                    Balance--;

                if (Balance == 0) break;
            }
        }
    }

    return Block;
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

            consume(p, TOKEN_IDENTIFIER);
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
        ParamCount++;

        token *ParamType = consume(p, TOKEN_IDENTIFIER);
        token *ParamName = consume(p, TOKEN_IDENTIFIER);

        ast_node *Param = node(p, NODE_VAR_DECL);

        Param->VarDecl.Init     = 0;
        Param->VarDecl.Name     = ParamName->String;
        Param->VarDecl.TypeName = ParamType->String;

        Result[i++] = Param;

        if (peek(p)->Type == TOKEN_COMMA) consume(p, TOKEN_COMMA);
    }

    consume(p, TOKEN_CLOSE_PAREN);

    return Result;
}

bool is_function(parser *p) {
    bool Result = (p->Tokens.Tokens[p->i + 0].Type == TOKEN_IDENTIFIER &&
                   p->Tokens.Tokens[p->i + 1].Type == TOKEN_IDENTIFIER &&
                   p->Tokens.Tokens[p->i + 2].Type == TOKEN_OPEN_PAREN);

    return Result;
}

ast_node *parse_function(parser *p) {
    int ParamCount = 0;

    token *ReturnType = consume(p, TOKEN_IDENTIFIER);
    token *FuncName   = consume(p, TOKEN_IDENTIFIER);

    ast_node **Params = get_params(p, &ParamCount);
    ast_node *Body    = parse_block(p);
    ast_node *Root    = node(p, NODE_FUNC_DEF);

    Root->FuncDef.Name       = FuncName->String;
    Root->FuncDef.ReturnType = ReturnType->String;
    Root->FuncDef.Params     = Params;
    Root->FuncDef.ParamCount = ParamCount;
    Root->FuncDef.Body       = Body;

    return Root;
}

int get_function_count(parser *p) {
    int Count = 0;
    int Saved = p->i;

    do {
        if (is_function(p)) Count++;
    } while (advance(p));

    p->i = Saved;

    return Count;
}

ast_node *parse(memory_arena *arena, token_list tokens) {
    parser p = {0, 0, tokens, arena};

    ast_node *Root = node(&p, NODE_PROGRAM);

    Root->Program.FunctionCount = get_function_count(&p);
    Root->Program.Functions  = arena_push(arena, Root->Program.FunctionCount * sizeof(ast_node *));
    Root->Program.GlobalVars = 0;

    int i = 0;

    while (true) {
        if (is_function(&p)) {
            Root->Program.Functions[i++] = parse_function(&p);
        } else {
            if (!advance(&p)) break;
        }
    }

    return Root;
}

void print_function_node(ast_node *node) {
    if (node->Type != NODE_FUNC_DEF) return;

    printf("FUNCTION ");
    string_print(node->FuncDef.Name);
    printf("  (returns): ");
    string_print(node->FuncDef.ReturnType);
    printf(",  (params): ");
    for (int i = 0; i < node->FuncDef.ParamCount; i++) {
        string_print(node->FuncDef.Params[i]->VarDecl.TypeName);
        printf(" ");
        string_print(node->FuncDef.Params[i]->VarDecl.Name);

        if (i < node->FuncDef.ParamCount - 1) {
            printf(", ");
        }
    }
    printf("\n");
}

void print_node(ast_node *node) {
    printf("[NODE] :: ");

    switch (node->Type) {
        case NODE_FUNC_DEF:
            print_function_node(node);
            break;
        case NODE_RETURN:
            printf("return ");

            ast_node *RetVal = node->Return.Value;

            if (RetVal->Type == NODE_IDENT)
                string_print(RetVal->Ident.Name);
            else if (RetVal->Type == NODE_STRING_LIT)
                string_print(RetVal->StringLit.Value);
            else if (RetVal->Type == NODE_INT_LIT)
                printf("%lld", RetVal->IntegerLit.Value);

            printf("\n");
            break;
        case NODE_VAR_DECL:
            string_print(node->VarDecl.TypeName);
            printf(" ");
            string_print(node->VarDecl.Name);
            printf(" = ");
            printf("%lld\n", node->VarDecl.Init->IntegerLit.Value);
            break;
        default:
            printf("(didn't implement printing for node type %d yet!)\n", node->Type);
            break;
    }
}

void print_at(parser *p) {
    printf("==(Debug Trace)==\n");

    for (int i = -3; i <= 3; i++) {
        int idx = p->i + i;

        if (idx < 0) continue;

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
