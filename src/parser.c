#include "parser.h"

#include <stdarg.h>

#include "util/util.h"

typedef struct {
    size_t i;
    int Depth;
    token_list Tokens;
    memory_arena *Arena;
} parser;

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

    print_stack_trace();

    va_end(Args);
}

token *advance(parser *p) {
    if (has_next(p)) return &p->Tokens.Tokens[p->i++];

    return NULL;
}

token *back(parser *p) {
    if (p->i > 0) return &p->Tokens.Tokens[p->i--];

    return NULL;
}

token *peek(parser *p) { return &p->Tokens.Tokens[p->i]; }

token *peek_next(parser *p, int ahead) {
    if (p->i + ahead < p->Tokens.Count) return &p->Tokens.Tokens[p->i + ahead];

    return NULL;
}

token *consume(parser *p, token_type expected) {
    token *Tok = peek(p);
    if (Tok->Type != expected) {
        parse_error(p, "Expected %s, got %s\n", token_name(expected), token_name(Tok->Type));
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
    ast_node *Root = node(p, NODE_ASS);

    token *Left = consume(p, TOKEN_IDENTIFIER);
    (void)Left;
    consume(p, TOKEN_EQUALS);

    return Root;
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
// are some kind of expression.
//
// else, we create the node and return it.
ast_node *parse_expression(parser *p, bool peeking) {
    int Saved = p->i;

    ast_node *Node = 0;

    if (peeking) {
        // If peeking, we just want to return a non-zero value;
        // (-1) as a pointer will just yield 0xffffff...
        Node = (ast_node *)(-1);
    }

    token *Curr = advance(p);

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

                    // TODO: parse_identifier(p)?
                    ast_node *Identifier   = node(p, NODE_IDENT);
                    Identifier->Ident.Name = RetVal->String;

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

                    ast_node *Literal = parse_literal(p);

                    Node               = node(p, NODE_RETURN);
                    Node->Return.Value = Literal;
                } else {
                    parse_error(p, "Expected identifier after return, but got %s\n");
                    string_print(peek(p)->String);
                    Node = 0;
                }
            }
            break;
        case TOKEN_IDENTIFIER:
            break;
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

ast_node *parse_block(parser *p) {
    consume(p, TOKEN_OPEN_SCOPE);

    if (parse_expression(p, true)) {
        ast_node *Expr = parse_expression(p, false);

        printf("GOT\n");
        print_node(Expr);
    } else {
        printf("DIDN'T GOT\n");
    }

    // parse return OR
    // parse assignment OR
    // parse call OR
    // parse if OR
    // parse while OR
    // parse return OR
    // parse block

    consume(p, TOKEN_CLOSE_SCOPE);

    return NULL;
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

ast_node *parse_function(parser *p) {
    int ParamCount = 0;

    token *ReturnType = consume(p, TOKEN_IDENTIFIER);
    token *FuncName   = consume(p, TOKEN_IDENTIFIER);

    ast_node **Params = get_params(p, &ParamCount);
    ast_node *Body    = parse_block(p);
    ast_node *Root    = node(p, NODE_FUNC_DEF);

    (void)Body;

    Root->FuncDef.Name       = FuncName->String;
    Root->FuncDef.ReturnType = ReturnType->String;
    Root->FuncDef.Params     = Params;
    Root->FuncDef.ParamCount = ParamCount;

    return Root;
}

ast_node *parse(memory_arena *arena, token_list tokens) {
    parser p = {0, 0, tokens, arena};

    ast_node *Root = node(&p, NODE_PROGRAM);

    Root->Program.Functions  = arena_push(arena, 1 * sizeof(ast_node *));
    Root->Program.GlobalVars = 0;

    Root->Program.Functions[0] = parse_function(&p);

    /*
    // Do all functions!
    while (has_next(&p)) {
        // Either find a function or a declaration for a global variable.
        advance(&p);
    }
    */

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
        default:
            printf("(didn't implement printing for %d type yet!)\n", node->Type);
            break;
    }
}
