#pragma once

#include "string.h"
#include "arena.h"

#include <stdbool.h>
#include <execinfo.h>

#define ArraySize(arr) (sizeof(arr) / sizeof((arr)[0]))
#define Max(a, b) ((a) < (b) ? (b) : (a))

// Reset
#define ANSI_RESET "\033[0m"

// Text Styles
#define ANSI_BOLD "\033[1m"
#define ANSI_DIM "\033[2m"
#define ANSI_UNDERLINE "\033[4m"
#define ANSI_BLINK "\033[5m"
#define ANSI_REVERSE "\033[7m"
#define ANSI_HIDDEN "\033[8m"

// Standard Foreground Colors
#define ANSI_FG_BLACK "\033[30m"
#define ANSI_FG_RED "\033[31m"
#define ANSI_FG_GREEN "\033[32m"
#define ANSI_FG_YELLOW "\033[33m"
#define ANSI_FG_BLUE "\033[34m"
#define ANSI_FG_MAGENTA "\033[35m"
#define ANSI_FG_CYAN "\033[36m"
#define ANSI_FG_WHITE "\033[37m"

// Bright Foreground Colors
#define ANSI_FG_BRIGHT_BLACK "\033[90m"
#define ANSI_FG_BRIGHT_RED "\033[91m"
#define ANSI_FG_BRIGHT_GREEN "\033[92m"
#define ANSI_FG_BRIGHT_YELLOW "\033[93m"
#define ANSI_FG_BRIGHT_BLUE "\033[94m"
#define ANSI_FG_BRIGHT_MAGENTA "\033[95m"
#define ANSI_FG_BRIGHT_CYAN "\033[96m"
#define ANSI_FG_BRIGHT_WHITE "\033[97m"

// Standard Background Colors
#define ANSI_BG_BLACK "\033[40m"
#define ANSI_BG_RED "\033[41m"
#define ANSI_BG_GREEN "\033[42m"
#define ANSI_BG_YELLOW "\033[43m"
#define ANSI_BG_BLUE "\033[44m"
#define ANSI_BG_MAGENTA "\033[45m"
#define ANSI_BG_CYAN "\033[46m"
#define ANSI_BG_WHITE "\033[47m"

// Bright Background Colors
#define ANSI_BG_BRIGHT_BLACK "\033[100m"
#define ANSI_BG_BRIGHT_RED "\033[101m"
#define ANSI_BG_BRIGHT_GREEN "\033[102m"
#define ANSI_BG_BRIGHT_YELLOW "\033[103m"
#define ANSI_BG_BRIGHT_BLUE "\033[104m"
#define ANSI_BG_BRIGHT_MAGENTA "\033[105m"
#define ANSI_BG_BRIGHT_CYAN "\033[106m"
#define ANSI_BG_BRIGHT_WHITE "\033[107m"

#define FG_COLOR(id) "\x1b[38;5;" #id "m" // gray from 232 to 255

string read_entire_file(memory_arena *arena, const char *fp);
bool output_data_to_file(string data, const char *filename);
void print_stack_trace(); 
