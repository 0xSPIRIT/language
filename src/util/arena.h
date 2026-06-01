#pragma once

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define GB(x) (1024*1024*1024*((size_t) x))

static long PageSize = 0;

typedef struct {
    char *Data;
    size_t Used, Reserved, Capacity;
} memory_arena;

memory_arena make_arena();
void free_arena(memory_arena *a);
void *arena_push(memory_arena *a, size_t size);
