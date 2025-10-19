#include "ast.h"
#include "runtime.h"
#include "eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Ast *root;
extern int yyparse();
extern FILE *yyin;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <script.iml> [--dump-ast]\n", argv[0]);
        return 1;
    }
    int dump = 0;
    if (argc > 2 && strcmp(argv[2], "--dump-ast") == 0) dump = 1;

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("fopen");
        return 1;
    }

    if (yyparse() != 0) {
        printf("Parse failed\n");
        return 1;
    }
    fclose(yyin);

    if (dump) dump_ast(root, 0);

    eval_program(root);

    free_ast(root);
    return 0;
}
