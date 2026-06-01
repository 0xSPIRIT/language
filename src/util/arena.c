#include "arena.h"

#include <assert.h>
#include <stdio.h>

memory_arena make_arena() {
    memory_arena Result = {};

    Result.Capacity = GB(8);
    Result.Data     = (char *)mmap(NULL, Result.Capacity, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (!PageSize) {
        PageSize = sysconf(_SC_PAGESIZE);
    }

    return Result;
}

void free_arena(memory_arena *a) {
    // TODO: Release memory
}

void *arena_push(memory_arena *a, size_t size) {
    if (a->Used + size > a->Capacity) {
        assert(false);
        return NULL;
    }

    // Commit pages as we go
    size_t CommittedPages = a->Used / PageSize + (a->Used % PageSize > 0);
    size_t CommittedEnd   = (a->Used + size) / PageSize + ((a->Used + size) % PageSize > 0);

    if (CommittedEnd - CommittedPages > 0) {
        // Commit the extra pages required to hold the new allocation
        size_t Size = (CommittedEnd - CommittedPages) * PageSize;
        int Result  = mprotect(a->Data + (CommittedPages * PageSize), Size, PROT_READ | PROT_WRITE);

        if (Result == -1) {
            fprintf(stderr, "mprotect() error");
            exit(1);
        }
    }

    char *Result = a->Data + a->Used;

    a->Used += size;

    return Result;
}
