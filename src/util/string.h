#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "arena.h"

#define CSTR(x) ((string){(char *)x, sizeof(x) - 1})

typedef struct {
    char *Data;
    size_t Length;
} string;

string string_make(memory_arena *a, size_t capacity);
void string_print(string s);
void string_print_b(string s); // used for identifiers
void string_print_b2(string s); // generally used for types
void string_print_b3(string s); // function names
void string_print_b4(string s); // literals
void string_print_fmt(string s, int width);
bool string_equals(string a, string b);
char *string_to_cstr(string s);
long long string_to_ll(string s);
