#include "util.h"

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

bool output_data_to_file(string data, const char *filename) {
    FILE *File = fopen(filename, "wb");

    size_t Written = fwrite(data.Data, 1, data.Length, File);

    if (Written < data.Length) {
        printf("Error ocurred writing to file.\n");
        fclose(File);
        return false;
    }

    fclose(File);
    return true;
}

void print_stack_trace(void) {
    void *buffer[30];
    int size;

    // Get the addresses of the functions currently on the stack
    size = backtrace(buffer, 30);

    // Print the symbols (function names/addresses) to standard error
    fprintf(stderr, "--- Stack Trace (%d frames) ---\n", size);
    backtrace_symbols_fd(buffer, size, STDERR_FILENO);
}
