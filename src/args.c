#include <stdio.h>
#include <string.h>

#include "util/string.h"

typedef struct {
    string Filepath;
    bool Close;
    bool PrintTree;
} arg_result;

void help() {
    const char *TulipText =
        "\n\n\
   /$$               /$$ /$$          \n\
  | $$              | $$|__/          \n\
 /$$$$$$   /$$   /$$| $$ /$$  /$$$$$$ \n\
|_  $$_/  | $$  | $$| $$| $$ /$$__  $$\n\
  | $$    | $$  | $$| $$| $$| $$  \\ $$\n\
  | $$ /$$| $$  | $$| $$| $$| $$  | $$\n\
  |  $$$$/|  $$$$$$/| $$| $$| $$$$$$$/\n\
   \\___/   \\______/ |__/|__/| $$____/ \n\
                            | $$      \n\
                            | $$      \n\
                            |__/      \n\
                            ";

    puts(TulipText);
    printf("\ntulip is a C-like langauge!\nCompile a program by using ./tulip <program.tulip>\n");
}

arg_result handle_args(int argc, char **argv) {
    arg_result Result = {};

    if (argc == 1) {
        help();
        Result.Close = true;
    } else {
        for (int i = 1; i < argc; i++) {
            if (0 == strcmp(argv[i], "-tree")) {
                Result.PrintTree = true;
            } else if (0 == strcmp(argv[i], "-help")) {
                help();
                Result.Close = true;
            } else {
                Result.Filepath = (string){argv[i], strlen(argv[i])};
            }
        }
    }

    return Result;
}
