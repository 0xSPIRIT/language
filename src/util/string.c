#include "string.h"

#include "util.h"

#include <assert.h>

string string_make(memory_arena *a, size_t capacity) {
    string Result;

    Result.Data = arena_push(a, capacity);
    Result.Length = 0;

    return Result;
}

void string_print(string s) {
    printf("%.*s", (int)s.Length, s.Data);
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

    for (size_t i = 0; i < s.Length; i++)
        Result[i] = s.Data[i];

    Result[s.Length] = 0;

    return Result;
}
