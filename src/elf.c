#include <elf.h>
#include <stdio.h>
#include <stdlib.h>

void generate_elf() {
    uint32_t BaseAddr = 0x08048000;

    uint32_t CodeOffset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr);

    uint8_t ProgramData[] = {
        0xB8, 0x01, 0x00, 0x00, 0x00, // mov eax, 1 (set syscall to 1)
        0xBB, 0x43, 0x00, 0x00, 0x00, // mov ebx, 67 (set ebx to 67)
        0xCD, 0x80, // int 0x80 (do syscall)
    };

    uint32_t FileSize = sizeof(ProgramData);

    // 1. Create ELF Header (32 bit)
    Elf32_Ehdr ElfHeader = {
        .e_ident     = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS32, ELFDATA2LSB, EV_CURRENT, ELFOSABI_NONE},
        .e_type      = ET_EXEC,
        .e_machine   = EM_386,
        .e_version   = EV_CURRENT,
        .e_entry     = BaseAddr + CodeOffset,
        .e_phoff     = sizeof(Elf32_Ehdr),
        .e_shoff     = 0,
        .e_flags     = 0,
        .e_ehsize    = sizeof(Elf32_Ehdr),
        .e_phentsize = sizeof(Elf32_Phdr),
        .e_phnum     = 1,
        .e_shentsize = sizeof(Elf32_Shdr),
        .e_shnum     = 0,
        .e_shstrndx  = SHN_UNDEF
    };

    // 2. Create Program Header
    Elf32_Phdr ProgramHeader = {
        .p_type   = PT_LOAD,
        .p_offset = 0,
        .p_vaddr  = BaseAddr,  // Base address of the program, convention for 32-bit.
        .p_paddr  = 0,
        .p_filesz = FileSize + CodeOffset,
        .p_memsz  = FileSize + CodeOffset,
        .p_flags  = PF_X | PF_R,
        .p_align  = 0x1000,  // p_vaddr = p_offset (mod p_align)
    };

    FILE *File = fopen("output", "wb");

    if (!File) {
        fprintf(stderr, "Couldn't open output file.\n");
        exit(1);
    }

    fwrite(&ElfHeader, sizeof(ElfHeader), 1, File);
    fwrite(&ProgramHeader, sizeof(ProgramHeader), 1, File);
    fwrite(ProgramData, 1, FileSize, File);

    fclose(File);
}
