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

    return (string){Buffer, Count};
}

string get_filepath(string Filename) {
    string Result = Filename;

    for (int i = Result.Length - 1; i >= 0; i--) {
        if (Result.Data[i] == '/') {
            Result.Length = Max(0, i + 1);
            break;
        }
    }

    return Result;
}

string get_filename_from_path(string Filepath) {
    string Result = Filepath;

    for (int i = Result.Length - 1; i >= 0; i--) {
        if (Result.Data[i] == '/') {
            Result.Data += i + 1;
            Result.Length -= i + 1;
            break;
        }
    }

    return Result;
}

string strip_file_extension(string Filename) {
    string Result = Filename;

    for (int i = Result.Length - 1; i >= 0; i--) {
        if (Result.Data[i] == '.') {
            Result.Length = i;
            break;
        }
    }

    return Result;
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
