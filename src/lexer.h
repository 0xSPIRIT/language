#pragma once

#include "util/arena.h"
#include "util/string.h"
#include "util/util.h"

typedef enum {
    TOKEN_NONE       = 0,
    TOKEN_NUMBER     = 1,
    TOKEN_IDENTIFIER = 2,
    TOKEN_KEYWORD    = 3,
    TOKEN_STRING_LIT = 4,

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
    TOKEN_COMMA         = ',',
    TOKEN_QUOTE         = '"',
} token_type;

typedef enum {
    KEYWORD_INVALID = 0,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_SWITCH,
    KEYWORD_STRUCT,
    KEYWORD_RETURN
} keyword;

const string Keywords[] = {
    CSTR("if"), CSTR("else"), CSTR("switch"), CSTR("struct"), CSTR("return"),
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

string get_keyword_str(keyword Keyword);
keyword keyword_from_index(int Index);

token_list tokenize(memory_arena *arena, string code, string filename);
