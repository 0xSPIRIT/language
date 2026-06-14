#include <elf.h>
#include <stdio.h>
#include <stdlib.h>

#include "gen_x86.h"

void generate_elf_x86(program_code program) {
    uint32_t BaseAddr = 0x400000;

    uint32_t CodeOffset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);

    // 1. Create ELF Header (64-bit)
    Elf64_Ehdr ElfHeader = {
        .e_ident     = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE},
        .e_type      = ET_EXEC,
        .e_machine   = EM_X86_64,
        .e_version   = EV_CURRENT,
        .e_entry     = BaseAddr + CodeOffset,
        .e_phoff     = sizeof(Elf64_Ehdr),
        .e_shoff     = 0,
        .e_flags     = 0,
        .e_ehsize    = sizeof(Elf64_Ehdr),
        .e_phentsize = sizeof(Elf64_Phdr),
        .e_phnum     = 1,
        .e_shentsize = sizeof(Elf64_Shdr),
        .e_shnum     = 0,
        .e_shstrndx  = SHN_UNDEF
    };

    // 2. Create Program Header
    Elf64_Phdr ProgramHeader = {
        .p_type   = PT_LOAD,
        .p_flags  = PF_X | PF_R,
        .p_offset = 0,
        .p_vaddr  = BaseAddr,
        .p_paddr  = 0,
        .p_filesz = program.DataSize + CodeOffset,
        .p_memsz  = program.DataSize + CodeOffset,
        .p_align  = 0x1000,
    };

    FILE *File = fopen("output", "wb");

    if (!File) {
        fprintf(stderr, "Couldn't open output file.\n");
        exit(1);
    }

    fwrite(&ElfHeader, sizeof(ElfHeader), 1, File);
    fwrite(&ProgramHeader, sizeof(ProgramHeader), 1, File);
    fwrite(program.Data, 1, program.DataSize, File);

    fclose(File);
}
