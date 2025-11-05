#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

void codegen_program(Ast *prog, const char *output_c_filename);

#endif