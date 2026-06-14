#pragma once

#include "util/arena.h"
#include "util/string.h"

typedef enum {
    TOKEN_NONE       = 0,
    TOKEN_NUMBER     = 1,
    TOKEN_IDENTIFIER = 2,
    TOKEN_KEYWORD    = 3,
    TOKEN_STRING_LIT = 4,
    TOKEN_CHAR_LIT   = 5,

    TOKEN_EQUALS_EQUALS,
    TOKEN_OR,
    TOKEN_AND,
    TOKEN_BANG_EQUALS,
    TOKEN_LESS_EQUALS,
    TOKEN_MORE_EQUALS,

    TOKEN_INC,
    TOKEN_DEC,

    TOKEN_PLUS_EQ,
    TOKEN_MINUS_EQ,
    TOKEN_TIMES_EQ,
    TOKEN_DIV_EQ,
    TOKEN_MOD_EQ,

    TOKEN_AMP           = '&',
    TOKEN_DOT           = '.',
    TOKEN_PERCENT       = '%',
    TOKEN_BANG          = '!',
    TOKEN_PLUS          = '+',
    TOKEN_MINUS         = '-',
    TOKEN_STAR          = '*',
    TOKEN_DIVIDE        = '/',
    TOKEN_EQUALS        = '=',
    TOKEN_LESS          = '<',
    TOKEN_MORE          = '>',
    TOKEN_OPEN_PAREN    = '(',
    TOKEN_CLOSE_PAREN   = ')',
    TOKEN_OPEN_SQUARE   = '[',
    TOKEN_CLOSE_SQUARE  = ']',
    TOKEN_OPEN_SCOPE    = '{',
    TOKEN_CLOSE_SCOPE   = '}',
    TOKEN_END_STATEMENT = ';',
    TOKEN_COMMA         = ',',
    TOKEN_QUOTE         = '"',
    TOKEN_CHAR_QUOTE    = '\'',
} token_type;

typedef enum {
    KEYWORD_INVALID = 0,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_FOR,
    KEYWORD_WHILE,
    KEYWORD_SWITCH,
    KEYWORD_STRUCT,
    KEYWORD_RETURN
} keyword;

const string Keywords[] = {
    CSTR("if"),     CSTR("else"),   CSTR("for"),    CSTR("while"),
    CSTR("switch"), CSTR("struct"), CSTR("return"),
};

typedef struct {
    token_type Type;
    string String;
    keyword Keyword;  // Index into Keywords[]
} token;

typedef struct {
    token *Tokens;
    size_t Count;
} token_list;

char *token_name(token_type type);

bool is_separator(char ch);
bool is_whitespace(char ch);
bool is_number(char ch);
bool is_letter(char ch);
bool is_single(char c);

void print_token(token *tok);

char *get_operation_str(token_type type);

bool is_token_binary_op(token_type type);

string get_keyword_str(keyword Keyword);
keyword keyword_from_index(int Index);

token_list tokenize(memory_arena *arena, string code, string filename);
