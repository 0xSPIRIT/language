#include <assert.h>
#include <stdint.h>

#include "gen.c"
#include "lexer.c"
#include "parser.c"
#include "sym.c"
#include "util/arena.c"
#include "util/string.c"
#include "util/util.c"

int main(int argc, char **argv) {
    memory_arena Arena = make_arena();

    string Filename;

    if (argc == 1)
        Filename = CSTR("../test/a.c");
    else
        Filename = (string){argv[1], strlen(argv[1])};

    string Code = read_entire_file(&Arena, Filename.Data);

    token_list Tokens = tokenize(&Arena, Code, Filename);

#define OUTPUT_ASM_TO_CONSOLE 0

    if (Tokens.Tokens) {
        ast_node *Tree = parse(&Arena, Tokens);

        resolve_symbols(Tree);

        // print_tree(Tree);

        string_print(Code);

#if OUTPUT_ASM_TO_CONSOLE
        FILE *out = stdout;
#else
        FILE *out = fopen("test.s", "w");
#endif

        program_code Program = gen_program_code(out, &Arena, Tree);

#if !OUTPUT_ASM_TO_CONSOLE
        fclose(out);

        printf("Assembling...\n");
        system("as -o test.o test.s");

        printf("Linking...\n");
        system("gcc -o test test.o");

        printf("Compilation completed. Output: ./test\n");
#endif

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
