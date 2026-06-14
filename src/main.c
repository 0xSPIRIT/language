#include <assert.h>
#include <stdint.h>

#include "lexer.c"
#include "parser.c"
#include "elf.c"
#include "gen_x86.c"

#include "util/arena.c"
#include "util/string.c"
#include "util/util.c"

int main(int argc, char **argv) {
    memory_arena Arena = make_arena();

    string Filename;

    if (argc == 1)
        Filename = CSTR("../test/a.l");
    else
        Filename = (string){argv[1], strlen(argv[1])};

    string Code = read_entire_file(&Arena, Filename.Data);

    token_list Tokens = tokenize(&Arena, Code, Filename);

    if (Tokens.Tokens) {
        ast_node *Tree = parse(&Arena, Tokens);
        print_tree(Tree);

        program_code Program = gen_x86(Tree);
        generate_elf_x86(Program);
    } else {
        printf("Error!\n");
    }


    free_arena(&Arena);
    return 0;
}
