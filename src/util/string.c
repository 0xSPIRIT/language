#include "string.h"

#include <assert.h>
#include <string.h>

#include "util.h"

string string_make(memory_arena *a, size_t capacity) {
    string Result;

    Result.Data   = arena_push(a, capacity);
    Result.Length = 0;

    return Result;
}

void string_print(string s) { printf("%.*s", (int)s.Length, s.Data); }

void string_print_to(FILE *out, string s) { fprintf(out, "%.*s", (int)s.Length, s.Data); }

void string_print_b(string s) {
    printf(ANSI_FG_BLUE);
    printf("%.*s", (int)s.Length, s.Data);
    printf(ANSI_RESET);
}

void string_print_b2(string s) {
    printf(ANSI_FG_YELLOW);
    printf("%.*s", (int)s.Length, s.Data);
    printf(ANSI_RESET);
}

void string_print_b3(string s) {
    printf(ANSI_FG_GREEN);
    printf("%.*s", (int)s.Length, s.Data);
    printf(ANSI_RESET);
}

void string_print_b4(string s) {
    printf(ANSI_FG_MAGENTA);
    printf("%.*s", (int)s.Length, s.Data);
    printf(ANSI_RESET);
}

void string_print_fmt(string s, int width) {
    string_print(s);
    if (s.Length < width) {
        int Spaces = width - s.Length;
        printf("%*s", Spaces, "");
    }
}

long long string_to_ll(string s) {
    char str[128];
    strncpy(str, s.Data, s.Length);
    return atoll(str);
}

bool string_equals(string a, string b) {
    if (b.Length != a.Length) return false;

    for (size_t i = 0; i < a.Length; i++)
        if (a.Data[i] != b.Data[i]) return false;

    return true;
}

void string_append(string *dest, const string src) {
    for (size_t i = 0; i < src.Length; i++) {
        dest->Data[dest->Length + i] = src.Data[i];
    }

    dest->Length += src.Length;
}

char *string_to_cstr(string s) {
    static char Result[1024] = {};

    assert(s.Length < ArraySize(Result));

    for (size_t i = 0; i < s.Length; i++) Result[i] = s.Data[i];

    Result[s.Length] = 0;

    return Result;
}
