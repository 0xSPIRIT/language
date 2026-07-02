#include <assert.h>
#include <stdint.h>

#include "lexer.c"
#include "parser.c"
#include "gen.c"
#include "sym.c"

#include "util/arena.c"
#include "util/string.c"
#include "util/util.c"

int main(int argc, char **argv) {
    memory_arena Arena = make_arena();

    string Filename;

    if (argc == 1)
        Filename = CSTR("../test/a");
    else
        Filename = (string){argv[1], strlen(argv[1])};

    string Code = read_entire_file(&Arena, Filename.Data);

    token_list Tokens = tokenize(&Arena, Code, Filename);

    if (Tokens.Tokens) {
        ast_node *Tree = parse(&Arena, Tokens);

        resolve_symbols(Tree);

        //print_tree(Tree);

        program_code Program = gen_program_code(&Arena, Tree);

        /*
        FILE *f = fopen("output.s", "wb");

        if (f) {
            fwrite(Program.ProgramCode.Data, 1, Program.ProgramCode.Length, f);
            fclose(f);
        } else {
            fprintf(stderr, "Error opening output file.\n");
            return 1;
        }

        system("as -o output.o output.s");

        system("ld -o output output.o");
        */

        free_program_code(&Program);
    } else {
        printf("Error!\n");
    }

    free_arena(&Arena);
    return 0;
}
