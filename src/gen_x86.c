#include "gen_x86.h"

#include <string.h>

program_code gen_x86(ast_node *ast) {
    program_code Program = {};

    Program.Arena = make_arena();
    Program.Data  = (uint8_t *)Program.Arena.Data;

    uint8_t SampleProgramData[] = {
        0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, 1 (set syscall to 1)
        0xBB, 0x43, 0x00, 0x00, 0x00, // mov ebx, 67 (set ebx to 67)
        0xCD, 0x80, // int 0x80 (do syscall)
    };

    // Reserve enough space for the data
    arena_push(&Program.Arena, sizeof(SampleProgramData));
    memcpy(Program.Data, SampleProgramData, sizeof(SampleProgramData));
    Program.DataSize = sizeof(SampleProgramData);

    return Program;
}

void free_x86(program_code *program) { free_arena(&program->Arena); }
