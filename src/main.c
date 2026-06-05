#include <assert.h>
#include <stdint.h>

#include "lexer.c"
#include "parser.c"

#include "util/arena.c"
#include "util/string.c"
#include "util/util.c"

#define TEST_ASM \
    "\
.globl main\n\
main:\n\
    movl    $67, %eax\n\
    ret\n"

int main() {
    memory_arena Arena = make_arena();

    const string Filename = CSTR("../test/a.l");

    string Code = read_entire_file(&Arena, Filename.Data);

    token_list Tokens = tokenize(&Arena, Code, Filename);

    if (Tokens.Tokens) {
        ast_node *Tree = parse(&Arena, Tokens);
        print_tree(Tree);
    } else {
        printf("Error!\n");
    }

    /*
    string AssemblyCode = string_make(&Arena, GB(2));

    string_append(&AssemblyCode, CSTR(TEST_ASM));

    string_print(AssemblyCode);

    output_data_to_file(AssemblyCode, "prog.s");
    */

    free_arena(&Arena);
    return 0;
}
