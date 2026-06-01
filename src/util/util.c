#include "string.h"
#include "arena.h"

string read_entire_file(memory_arena *arena, const char *fp) {
    FILE *file = fopen(fp, "r");

    if (!file) {
        fprintf(stderr, "Failed to open %s\n", fp);
        return (string){};
    }

    fseek(file, 0, SEEK_END);
    size_t Size = ftell(file);
    rewind(file);

    char *Buffer = arena_push(arena, Size + 1);

    if (!Buffer) {
        fclose(file);
        fprintf(stderr, "Error: memory arena too small.");
        return (string){};
    }

    size_t Count = fread(Buffer, 1, Size, file);

    fclose(file);

    return (string){ Buffer, Count };
}
