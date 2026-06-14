#include <assert.h>
#include <stdint.h>

#include "lexer.c"
#include "parser.c"
#include "elf.c"
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
    } else {
        printf("Error!\n");
    }

    generate_elf();

    free_arena(&Arena);
    return 0;
}
