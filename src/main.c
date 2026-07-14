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

    if (Tokens.Tokens) {
        ast_node *Tree = parse(&Arena, Tokens);

        resolve_symbols(Tree);

        // print_tree(Tree);

        // string_print(Code);

        constexpr bool OUTPUT_ASM_TO_CONSOLE = false;

        FILE *Out;

        if (OUTPUT_ASM_TO_CONSOLE)
            Out = stdout;
        else
            Out = fopen("test.s", "w+");

        program_code Program = gen_program_code(Out, &Arena, Tree);

        if (!OUTPUT_ASM_TO_CONSOLE) {
            long Size = ftell(Out);

            fseek(Out, 0, SEEK_SET);

            char *Data = malloc(Size + 1);
            Data[Size] = 0;
            fread(Data, 1, Size, Out);
            puts(Data);
            fclose(Out);
            free(Data);

            int result;

            printf("Assembling...\n");
            result = system("as --gdwarf-5 -o test.o test.s");

            if (!result) {
                printf("Assembler successful.\n");

                printf("Linking...\n");
                result = system("ld -o test test.o");

                if (!result) {
                    printf("Compilation completed. Output: ./test\n");
                } else {
                    printf("Compilation failed.\n");
                }
            } else {
                printf("Assembler failed!\n");
            }
        }

        free_program_code(&Program);
    } else {
        printf("Error!\n");
    }

    free_arena(&Arena);
    return 0;
}
