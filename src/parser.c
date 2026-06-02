#include "parser.h"

#include <stdarg.h>

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

    va_end(Args);
}

token *advance(parser *p) {
    if (has_next(p))
        return &p->Tokens.Tokens[p->i++];

    return NULL;
}

token *peek(parser *p) {
    return &p->Tokens.Tokens[p->i];
}

token *peek_next(parser *p, int ahead) {
    if (p->i + ahead < p->Tokens.Count)
        return &p->Tokens.Tokens[p->i + ahead];

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

ast_node *parse_expression(parser *p) {
    return NULL;
}

ast_node *parse_assignment(parser *p) {
    ast_node *Root = node(p, NODE_ASS);

    token *Left = consume(p, TOKEN_IDENTIFIER);
    consume(p, TOKEN_EQUALS);

    return Root;
}

ast_node *parse_block(parser *p) {
    consume(p, TOKEN_OPEN_SCOPE);

    // parse assignment OR
    // parse call OR
    // parse if OR
    // parse while OR
    // parse return OR
    // parse block

    consume(p, TOKEN_CLOSE_SCOPE);

    return NULL;
}

ast_node *parse_function(parser *p) {
    ast_node *Root = node(p, NODE_FUNC_DEF);

    token *ReturnType = consume(p, TOKEN_IDENTIFIER);
    token *FuncName = consume(p, TOKEN_IDENTIFIER);

    // TODO get_params() function
    consume(p, TOKEN_OPEN_PAREN);
    consume(p, TOKEN_CLOSE_PAREN);

    ast_node *Body = parse_block(p);

    return Root;
}

ast_node *parse(memory_arena *arena, token_list tokens) {
    parser p = {0, 0, tokens, arena};

    ast_node *Tree = node(&p, NODE_PROGRAM);

    // Do all functions!

    while (has_next(&p)) {
        advance(&p);
    }

    return Tree;
}
