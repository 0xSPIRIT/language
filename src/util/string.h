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
bool string_equals(string a, string b);
