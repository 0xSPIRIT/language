#include "parser.h"

ast_node *parse(token_list tokens) {
    ast_node *tree = 0;

    for (size_t i = 0; i < tokens.Count; i++) {
        string_print(tokens.Tokens[i].String);
    }

    return tree;
}
