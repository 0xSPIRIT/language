#pragma once

#include "lexer.h"

typedef struct ast_node {
    int _;
} ast_node;

ast_node *parse(token_list tokens);
