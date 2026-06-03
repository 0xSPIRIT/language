#pragma once

#include "string.h"
#include "arena.h"

#include <stdbool.h>
#include <execinfo.h>

#define ArraySize(arr) (sizeof(arr) / sizeof((arr)[0]))

string read_entire_file(memory_arena *arena, const char *fp);
bool output_data_to_file(string data, const char *filename);
void print_stack_trace(); 
