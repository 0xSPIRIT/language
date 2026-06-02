#include "lexer.h"

char *token_name(token_type type) {
    switch (type) {
        case TOKEN_NONE:
            return "none";
        case TOKEN_NUMBER:
            return "number";
        case TOKEN_IDENTIFIER:
            return "identifier";
        case TOKEN_KEYWORD:
            return "keyword";
        case TOKEN_STRING_LIT:
            return "string literal";
        case TOKEN_PLUS:
            return "+";
        case TOKEN_MINUS:
            return "-";
        case TOKEN_MULTIPLY:
            return "*";
        case TOKEN_DIVIDE:
            return "/";
        case TOKEN_EQUALS:
            return "=";
        case TOKEN_LESS:
            return "<";
        case TOKEN_MORE:
            return ">";
        case TOKEN_OPEN_PAREN:
            return "(";
        case TOKEN_CLOSE_PAREN:
            return ")";
        case TOKEN_OPEN_SQUARE:
            return "[";
        case TOKEN_CLOSE_SQUARE:
            return "]";
        case TOKEN_OPEN_SCOPE:
            return "{";
        case TOKEN_CLOSE_SCOPE:
            return "}";
        case TOKEN_END_STATEMENT:
            return ";";
        case TOKEN_QUOTE:
            return "\"";
        default:
            return "(unknown token)";
    }
}

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
        case '"':
            return true;
        default:
            return false;
    }
}

token_list tokenize(memory_arena *arena, string code, string filename) {
    // Overestimate the number of tokens.
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

        if (ch == '"') {
            int Start            = i;
            Tokens[TokenCount++] = (token){TOKEN_QUOTE, CSTR("\"")};

            i++;

            while (code.Data[i++] != '"') {
                if (i >= code.Length) {
                    printf("String didn't end (started at ");
                    string_print(filename);
                    printf(" line %zu, column %zu\n", Line, Col);
                }
            }

            i--;

            size_t StringLength = i - Start - 1;

            string Str = {code.Data + Start + 1, StringLength};

            Tokens[TokenCount++] = (token){TOKEN_STRING_LIT, Str};
            Tokens[TokenCount++] = (token){TOKEN_QUOTE, CSTR("\"")};

            continue;
        } else if (is_single(ch)) {
            token tok;
            tok.Type             = (token_type)ch;
            tok.String           = (string){code.Data + i, 1};
            Tokens[TokenCount++] = tok;

            continue;
        }

        printf("(%c) Syntax error in ", ch);
        string_print(filename);
        printf(" line %zu, column %zu\n", Line, Col);
    }

    return (token_list){Tokens, TokenCount};
}
