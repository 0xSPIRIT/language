#pragma once

#include "lexer.h"
#include "sym.h"
#include "type.h"

#define MAX_STATEMENTS 2048

#define PRINT_SYMBOLS_IN_TREE

#define consume(p, token) _consume(p, token, __FILE__, __LINE__)

typedef enum {
    NODE_INVALID = 0,
    NODE_PROGRAM,
    NODE_FUNC_DEF,
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_TYPE,
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
            struct ast_node **Functions;  // array of NODE_FUNC_DEF
            int FunctionCount;
            struct ast_node **GlobalDecls;  // array of NODE_VAR_DECL
            int GlobalDeclCount;
        } Program;

        // NODE_FUNC_DEF
        struct {
            struct ast_node *Name;  // NODE_IDENT
            struct ast_node *ReturnType;
            struct ast_node **Params;  // array of NODE_VAR_DECL
            int ParamCount;
            struct ast_node *Body;  // NODE_BLOCK
            // struct ast_node **Functions; // TODO: Implement storing nested functions here instead
            // of needing to walk through Body
        } FuncDef;

        // NODE_BLOCK
        struct {
            struct ast_node **Statements;
            int StatementCount;
        } Block;

        // NODE_STRUCT
        struct {
            struct ast_node *Name;     // NODE_IDENT
            struct ast_node **Fields;  // array of NODE_VAR_DECL
            int FieldCount;
        } Struct;

        // NODE_TYPE:    Example: [10]**int a => { Type = TYPE_ARR, PointingTo = { Type = TYPE_PTR,
        //               PointingTo = { TYPE_PTR, PointingTo = { Type = TYPE_S16, PointingTo = null
        //               }}}}
        struct {
            type Type;
            struct ast_node *Name;  // NODE_IDENT, nullable if builtin type
            int ArraySize;

            struct ast_node *PointingTo;  // An ast_node *DataType
        } DataType;

        // NODE_VAR_DECL
        struct {
            struct ast_node *Name;         // NODE_IDENT
            struct ast_node *Type;         // points to NODE_TYPE
            struct ast_node *Init;         // nullable
            struct ast_node **ChildDecls;  // int a, b, c; => b and c are NODE_VAR_DECL's stored here in a.
            int ChildDeclsCount;
        } VarDecl;

        // NODE_BINARY_OP
        struct {
            token_type Operation;
            struct ast_node *Left, *Right;
        } BinaryOp;

        // NODE_UNARY_OP
        struct {
            bool First;  // If true, then prefix unary (--a), If false, then postfix unary (a++)
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
            struct ast_node *FuncName; // NODE_IDENT
            struct ast_node **Args;
            int ArgCount;
        } Call;

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

        // NODE_IDENT
        struct {
            string Name;
            symbol *Sym;
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
ast_node *parse_var_decl(parser *p);
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
void print_node_type(ast_node *node);
void print_symbol_ptr(ast_node *node);
