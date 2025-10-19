#ifndef EVAL_H
#define EVAL_H

#include "ast.h"

// Function declarations
void eval_program(Ast *prog);
void eval_stmt(Ast *stmt);
Image *eval_expr(Ast *expr);

#endif
