#include <assert.h>
#include <stdint.h>

#include "args.c"
#include "gen.c"
#include "lexer.c"
#include "parser.c"
#include "sym.c"
#include "util/arena.c"
#include "util/string.c"
#include "util/util.c"

int main(int argc, char **argv) {
    memory_arena Arena = make_arena();

    arg_result ArgResult = handle_args(argc, argv);

    if (ArgResult.Close) {
        return 1;
    }

    string Filepath = ArgResult.Filepath;

    string Filename = strip_file_extension(get_filename_from_path(Filepath));

    string Code = read_entire_file(&Arena, Filepath.Data);

    memory_arena TokenArena = make_arena();
    token_list Tokens       = tokenize(&TokenArena, &Arena, Code, Filepath);

    if (!Tokens.Tokens) {
        printf("Memory error!\n");
        return 1;
    }

    printf("Parsing...\n");
    ast_node *Tree = parse(&Arena, Tokens);

    if (!Tree) {
        printf("Memory error!");
        return 1;
    }

    printf("Resolving symbols...\n");
    resolve_symbols(Tree);

    if (ArgResult.PrintTree) {
        print_tree(Tree);
    }

    FILE *Out;

    {
        char AsmFilename[2048];
        sprintf(AsmFilename, "%.*s.s", (int)Filename.Length, Filename.Data);
        Out = fopen(AsmFilename, "w");
    }

    printf("Compiling...\n");
    program_code Program = gen_program_code(Out, &Arena, Tree);

    fclose(Out);

    int result;

    printf("Assembling...\n");
    {
        char Cmd[2048];
        sprintf(Cmd, "as --gdwarf-5 -o %.*s.o %.*s.s", (int)Filename.Length, Filename.Data, (int)Filename.Length, Filename.Data);
        result = system(Cmd);
    }

    if (!result) {
        printf("Assembler successful.\n");

        printf("Linking...\n");
        {
            char Cmd[2048];
            sprintf(Cmd, "gcc -o %.*s %.*s.o\n", (int)Filename.Length, Filename.Data, (int)Filename.Length, Filename.Data);
            result = system(Cmd);
        }

        if (!result) {
            printf("Compilation completed. Output: ./%.*s\n", (int)Filename.Length, Filename.Data);
        } else {
            printf("Compilation failed.\n");
        }
    } else {
        printf("Assembler failed!\n");
    }

    free_program_code(&Program);

    free_arena(&Arena);
    return 0;
}
