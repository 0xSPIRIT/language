#pragma once

#include "lexer.h"

#define MAX_STATEMENTS 2048

#define consume(p, token) _consume(p, token, __FILE__, __LINE__)

typedef enum {
    NODE_INVALID = 0,
    NODE_PROGRAM,
    NODE_FUNC_DEF,
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_ASSIGN,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_STRUCT,
    NODE_CALL,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_IDENT,
    NODE_INT_LIT,
    NODE_FLOAT_LIT,
    NODE_STRING_LIT,
    NODE_CHAR_LIT,
} node_type;

typedef struct ast_node {
    node_type Type;

    union {
        // NODE_PROGRAM
        struct {
            struct ast_node **Functions;   // array of NODE_FUNC_DEF
            int FunctionCount;
            struct ast_node **GlobalVars;  // array of NODE_VAR_DECL
            int GlobalVarCount;
        } Program;

        // NODE_FUNC_DEF
        struct {
            string Name;
            string ReturnType;
            struct ast_node **Params;  // array of NODE_VAR_DECL
            int ParamCount;
            struct ast_node *Body;     // NODE_BLOCK
            //struct ast_node **Functions; // TODO: Implement storing nested functions here instead of needing to walk through Body
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
            struct ast_node *Left, *Right;
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

        // NODE_WHILE
        struct {
            struct ast_node *Condition, *Body;
        } While;

        // NODE_FOR
        struct {
            struct ast_node *Init, *Condition, *Advance, *Body;
        } For;

        // NODE_CALL
        struct {
            string FuncName;
            struct ast_node **Args;
            int ArgCount;
        } Call;

        // NODE_ASSIGN
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
            char Value;
        } CharLit;

        struct {
            string Name;
        } Ident;
    };

    int Line, Col;
} ast_node;

typedef struct {
    size_t i;
    int Depth;
    token_list Tokens;
    memory_arena *Arena;
} parser;

ast_node *node(parser *p, node_type type);

ast_node *parse(memory_arena *arena, token_list tokens);
void print_node(ast_node *node);
void print_at(parser *p);
ast_node *parse_block(parser *p);
ast_node *parse_function(parser *p);
ast_node *parse_function_call(parser *p);
bool is_function(parser *p);
ast_node *parse_expression(parser *p);
ast_node *parse_identifier(parser *p);
ast_node *parse_literal(parser *p);
token *peek(parser *p);
void parse_error(parser *p, const char *format, ...);
bool has_next(parser *p);
token *advance(parser *p);

bool is_function_call(parser *p);

node_type next_statement_type(parser *p);
ast_node *parse_statement(parser *p, node_type type);
token *_consume(parser *p, token_type expected, const char *_file, int _line);
void print_tree(ast_node *ast);
