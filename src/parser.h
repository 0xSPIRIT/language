#pragma once

#include "lexer.h"

typedef enum {
    NODE_PROGRAM,
    NODE_FUNC_DEF,
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_ASS,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_CALL,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_IDENT,
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_STRING_LIT
} node_type;

typedef struct ast_node {
    node_type Type;

    union {
        // NODE_FUNC_DEF
        struct {
            string Name;
            struct ast_node **Params;  // array of NODE_VAR_DECL
            int ParamCount;
        } FuncDef;

        // NODE_BLOCK
        struct {
            struct ast_node **Statements;
            int StatementCount;
        } Block;

        // NODE_VAR_DECL
        struct {
            string Name;
            string TypeName;
            struct ast_node *Init;  // nullable
        } VarDecl;

        // NODE_BINARY_OP
        struct {
            token_type Operation;
            struct ast_node *left, *right;
        } BinaryOp;

        // NODE_UNARY_OP
        struct {
            token_type Operation;
            struct ast_node *Operand;
        } UnaryOp;

        // NODE_IF
        struct {
            struct ast_node *Condition, *ThenBlock, *ElseBlock;
        } If;

        // NODE_CALL
        struct {
            string FuncName;
            struct ast_node **Args;
            int ArgCount;
        } Call;

        // NODE_ASS
        struct {
            string Target;
            struct ast_node *Value;
        } Assign;

        // NODE_RETURN
        struct {
            struct ast_node *Value;
        } Return;

        struct {
            long long Value;
        } IntegerLit;

        struct {
            double Value;
        } FloatLit;

        struct {
            string Value;
        } StringLit;

        struct {
            string Name;
        } Ident;
    };

    int Line, Col;
} ast_node;

ast_node *parse(memory_arena *arena, token_list tokens);
