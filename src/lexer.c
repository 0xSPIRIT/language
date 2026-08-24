#include "lexer.h"

#include <assert.h>
#include <string.h>

#include "util/util.h"

char *token_name(token_type type) {
    switch (type) {
        case TOKEN_NONE:          return "none";
        case TOKEN_NUMBER:        return "number";
        case TOKEN_IDENTIFIER:    return "identifier";
        case TOKEN_KEYWORD:       return "keyword";
        case TOKEN_STRING_LIT:    return "string literal";
        case TOKEN_CHAR_LIT:      return "char literal";
        case TOKEN_PLUS:          return "+";
        case TOKEN_MINUS:         return "-";
        case TOKEN_STAR:          return "*";
        case TOKEN_DIVIDE:        return "/";
        case TOKEN_EQUALS:        return "=";
        case TOKEN_LESS:          return "<";
        case TOKEN_MORE:          return ">";
        case TOKEN_OPEN_PAREN:    return "(";
        case TOKEN_CLOSE_PAREN:   return ")";
        case TOKEN_OPEN_SQUARE:   return "[";
        case TOKEN_CLOSE_SQUARE:  return "]";
        case TOKEN_OPEN_SCOPE:    return "{";
        case TOKEN_CLOSE_SCOPE:   return "}";
        case TOKEN_END_STATEMENT: return ";";
        case TOKEN_COMMA:         return ",";
        case TOKEN_QUOTE:         return "\"";
        case TOKEN_CHAR_QUOTE:    return "'";
        case TOKEN_AMP:           return "&";

        case TOKEN_EQUALS_EQUALS: return "==";
        case TOKEN_OR:            return "||";
        case TOKEN_AND:           return "&&";
        case TOKEN_BANG_EQUALS:   return "!=";
        case TOKEN_LESS_EQUALS:   return "<=";
        case TOKEN_MORE_EQUALS:   return ">=";
        case TOKEN_PERCENT:       return "%";
        case TOKEN_BANG:          return "!";
        case TOKEN_DOT:           return ".";

        case TOKEN_INC: return "++";
        case TOKEN_DEC: return "--";

        case TOKEN_PLUS_EQ:  return "+=";
        case TOKEN_MINUS_EQ: return "-=";
        case TOKEN_TIMES_EQ: return "*=";
        case TOKEN_DIV_EQ:   return "/=";
        case TOKEN_MOD_EQ:   return "%=";

        case TOKEN_ELLIPSES: return "...";

        default: return "(unknown token)";
    }
}

bool is_token_binary_op(token_type type) {
    switch (type) {
        case TOKEN_EQUALS:
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_DIVIDE:
        case TOKEN_LESS:
        case TOKEN_MORE:   return true;
        default:           return false;
    }
}

void print_token(token *tok) {
    printf("%-15s ", token_name(tok->Type));

    if (!is_single(tok->String.Data[0])) string_print(tok->String);
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
        case '\'':
        case ',':
        case '!':
        case '.':
        case '%':  return true;
        default:   return false;
    }
}

bool is_identifier_char(char ch) { return is_letter(ch) || ch == '_' || (ch >= '0' && ch <= '9'); }

keyword keyword_from_index(int Index) {
    keyword Result = Index + 1;
    return Result;
}

string get_keyword_str(keyword Keyword) { return Keywords[(int)(Keyword - 1)]; }

void push_token(memory_arena *arena, token Token) {
    token *Tok = arena_push(arena, sizeof(token));
    *Tok       = Token;
}

token_list tokenize(memory_arena *TokenArena, memory_arena *GeneralArena, string code, string filename) {
    size_t Line = 1;
    size_t Col  = 1;

    for (size_t i = 0; i < code.Length; i++) {
        char ch = code.Data[i];

        // Comment
        if (ch == '/' && i + 1 < code.Length && code.Data[i + 1] == '/') {
            while (i == code.Length || code.Data[i] != '\n') i++;

            continue;
        }

        // Preprocessor include
        if (ch == '#') {
            i++;

            const string Include = CSTR("include");
            string Str           = {code.Data + i, Include.Length};

            if (string_equals(Str, Include)) {
                i += Include.Length;

                while (is_whitespace(code.Data[i++]));
                assert(code.Data[i - 1] == '"');

                string IncludedFilename = {code.Data + i, 0};
                int Start               = i;

                while (code.Data[i] != '"' && code.Data[i] != '\n') i++;

                assert(code.Data[i] == '"');

                IncludedFilename.Length = i - Start;

                string Cwd = get_filepath(filename);

                char IncludedFilepath[256] = {};

                memcpy(IncludedFilepath, Cwd.Data, Cwd.Length);
                memcpy(IncludedFilepath + Cwd.Length, IncludedFilename.Data, IncludedFilename.Length);
                int Length = Cwd.Length + IncludedFilename.Length;

                string CodeForFile = read_entire_file(GeneralArena, IncludedFilepath);

                if (CodeForFile.Data) {
                    // tokens are pushed to the same arena, so we are good.
                    tokenize(TokenArena, GeneralArena, CodeForFile, (string){IncludedFilepath, Length});
                } else {
                    printf("Error: couldn't read file %.*s\n", (int)IncludedFilename.Length, IncludedFilename.Data);
                }
            }

            continue;
        }

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

            push_token(TokenArena, tok);

            continue;
        }

        string Sizeof = CSTR("sizeof");
        if (i + Sizeof.Length < code.Length) {
            string Str = {code.Data + i, Sizeof.Length};

            if (string_equals(Str, Sizeof)) {
                push_token(TokenArena, (token){TOKEN_SIZEOF, Str});
                i += Sizeof.Length - 1;
                continue;
            }
        }

        if (i + 2 < code.Length) {
            string Str = {code.Data + i, 3};

            if (string_equals(Str, CSTR("..."))) {
                push_token(TokenArena, (token){TOKEN_ELLIPSES, Str});
                i += 2;
                continue;
            }
        }

        if (is_letter(ch) || ch == '_') {
            size_t Start = i;
            while (i < code.Length && is_identifier_char(code.Data[i])) i++;
            i--;

            token tok;
            tok.Type   = TOKEN_NONE;
            tok.String = (string){code.Data + Start, i - Start + 1};

            for (int i = 0; i < ArraySize(Keywords); i++) {
                if (string_equals(tok.String, Keywords[i])) {
                    tok.Type    = TOKEN_KEYWORD;
                    tok.Keyword = keyword_from_index(i);
                }
            }

            if (!tok.Type) tok.Type = TOKEN_IDENTIFIER;

            push_token(TokenArena, tok);

            continue;
        }

        if (ch == '"') {
            int Start = i;

            push_token(TokenArena, (token){TOKEN_QUOTE, CSTR("\"")});

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

            push_token(TokenArena, (token){TOKEN_STRING_LIT, Str});
            push_token(TokenArena, (token){TOKEN_QUOTE, CSTR("\"")});

            continue;
        } else if (ch == '\'') {
            push_token(TokenArena, (token){TOKEN_CHAR_QUOTE, CSTR("'")});
            string Str = {code.Data + i + 1, 1};
            push_token(TokenArena, (token){TOKEN_CHAR_LIT, Str});
            push_token(TokenArena, (token){TOKEN_CHAR_QUOTE, CSTR("'")});

            i += 2;
            continue;
        } else if (i + 1 < code.Length) {
            string Str = {code.Data + i, 2};

            token_type Type = 0;

            if (string_equals(Str, CSTR("==")))
                Type = TOKEN_EQUALS_EQUALS;
            else if (string_equals(Str, CSTR("!=")))
                Type = TOKEN_BANG_EQUALS;
            else if (string_equals(Str, CSTR("<=")))
                Type = TOKEN_LESS_EQUALS;
            else if (string_equals(Str, CSTR(">=")))
                Type = TOKEN_MORE_EQUALS;
            else if (string_equals(Str, CSTR("&&")))
                Type = TOKEN_AND;
            else if (string_equals(Str, CSTR("||")))
                Type = TOKEN_OR;
            else if (string_equals(Str, CSTR("+=")))
                Type = TOKEN_PLUS_EQ;
            else if (string_equals(Str, CSTR("-=")))
                Type = TOKEN_MINUS_EQ;
            else if (string_equals(Str, CSTR("*=")))
                Type = TOKEN_TIMES_EQ;
            else if (string_equals(Str, CSTR("/=")))
                Type = TOKEN_DIV_EQ;
            else if (string_equals(Str, CSTR("%=")))
                Type = TOKEN_MOD_EQ;
            else if (string_equals(Str, CSTR("++")))
                Type = TOKEN_INC;
            else if (string_equals(Str, CSTR("--")))
                Type = TOKEN_DEC;

            if (Type) {
                push_token(TokenArena, (token){Type, Str});
                i++;
            } else {
                goto single;
            }

            continue;
        } else if (is_single(ch)) {
        single:
            token tok;
            tok.Type   = (token_type)ch;
            tok.String = (string){code.Data + i, 1};

            push_token(TokenArena, tok);

            continue;
        }

        printf("(%c) Syntax error in ", ch);
        string_print(filename);
        printf(" line %zu, column %zu\n", Line, Col);
    }

    token_list Result = {(token *)TokenArena->Data, TokenArena->Used / sizeof(token)};

    return Result;
}
