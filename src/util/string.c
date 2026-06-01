#include "string.h"

void string_print(string s) {
    printf("%.*s\n", (int)s.Length, s.Data);
}

bool string_equals(string a, string b) {
    if (b.Length != a.Length) return false;

    for (size_t i = 0; i < a.Length; i++)
        if (a.Data[i] != b.Data[i]) return false;

    return true;
}
