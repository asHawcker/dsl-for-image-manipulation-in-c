#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include "ast.h"

// Main entry point for optimization. Traverses the AST and rewrites nodes.
Ast *optimize_ast(Ast *root);

#endif