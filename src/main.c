#include <assert.h>
#include <stdint.h>

#include "lexer.c"
#include "parser.c"
#include "util/arena.c"
#include "util/string.c"
#include "util/util.c"

int main() {
    memory_arena Arena = make_arena();

    const string Filename = CSTR("../test/a.l");

    string Code = read_entire_file(&Arena, Filename.Data);

    token_list Tokens = tokenize(&Arena, Code, Filename);

    ast_node *Node = parse(Tokens);
    (void)Node;

    free_arena(&Arena);
    return 0;
}
