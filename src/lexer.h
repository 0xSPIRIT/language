#pragma once

#include "util/arena.h"
#include "util/string.h"

#define ArraySize(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef enum {
    TOKEN_NONE       = 0,
    TOKEN_NUMBER     = 1,
    TOKEN_IDENTIFIER = 2,
    TOKEN_KEYWORD    = 3,

    TOKEN_PLUS          = '+',
    TOKEN_MINUS         = '-',
    TOKEN_MULTIPLY      = '*',
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
} token_type;

const string Keywords[] = {
    CSTR("func"), CSTR("if"),     CSTR("else"),   CSTR("switch"),
    CSTR("int"),  CSTR("string"), CSTR("struct"), CSTR("return"),
};

typedef struct {
    token_type Type;
    string String;
} token;

typedef struct {
    token *Tokens;
    size_t Count;
} token_list;

bool is_separator(char ch);
bool is_whitespace(char ch);
bool is_number(char ch);
bool is_letter(char ch);
bool is_single(char c);

token_list tokenize(memory_arena *arena, string code, string filename);
