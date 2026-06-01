#include "lexer.h"

bool is_separator(char ch) {
    return ch == ' ' || ch == '\n' || ch == '(' || ch == ')' || ch == '{' || ch == '}';
}

bool is_whitespace(char ch) { return ch == ' ' || ch == '\n'; }

bool is_number(char ch) { return ch >= '0' && ch <= '9'; }

bool is_letter(char ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); }

bool is_single(char c) {
    switch (c) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '=':
        case '<':
        case '>':
        case '(':
        case ')':
        case '[':
        case ']':
        case '{':
        case '}':
        case ';':
            return true;
        default:
            return false;
    }
}

token_list tokenize(memory_arena *arena, string code, string filename) {
    token *Tokens     = (token *)arena_push(arena, sizeof(token) * code.Length);
    size_t TokenCount = 0;

    size_t Line = 1;
    size_t Col  = 1;

    if (!Tokens) {
        return (token_list){};
    }

    for (size_t i = 0; i < code.Length; i++) {
        char ch = code.Data[i];

        if (ch == '\n') {
            Col = 1;
            Line++;
        } else {
            Col++;
        }

        if (is_whitespace(ch)) continue;

        if (is_number(ch)) {
            size_t Start = i;
            while (i < code.Length && is_number(code.Data[i])) i++;
            i--;

            token tok;
            tok.Type   = TOKEN_NUMBER;
            tok.String = (string){code.Data + Start, i - Start + 1};

            Tokens[TokenCount++] = tok;

            continue;
        }

        if (is_letter(ch)) {
            size_t Start = i;
            while (i < code.Length && is_letter(code.Data[i])) i++;
            i--;

            token tok;
            tok.Type   = TOKEN_NONE;
            tok.String = (string){code.Data + Start, i - Start + 1};

            for (int i = 0; i < ArraySize(Keywords); i++) {
                if (string_equals(tok.String, Keywords[i])) {
                    tok.Type = TOKEN_KEYWORD;
                }
            }

            if (!tok.Type) tok.Type = TOKEN_IDENTIFIER;

            Tokens[TokenCount++] = tok;

            continue;
        }

        if (is_single(ch)) {
            token tok;
            tok.Type             = (token_type)ch;
            tok.String           = (string){code.Data + i, 1};
            Tokens[TokenCount++] = tok;

            continue;
        }

        printf("Syntax error in ");
        string_print(filename);
        printf(" line %zu, column %zu\n", Line, Col);
    }

    return (token_list){Tokens, TokenCount};
}
