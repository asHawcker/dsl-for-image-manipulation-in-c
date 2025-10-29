#ifndef EVAL_H
#define EVAL_H

#include "ast.h"
#include "runtime.h" // <-- Must include for 'Image' struct definition

// --- NEW VALUE AND TYPE DEFINITIONS ---

typedef enum {
    V_INT,
    V_FLOAT,
    V_STRING,
    V_IMAGE,
    V_NONE
} ValueType;

typedef struct Value {
    ValueType tag;
    union {
        int ival;
        double fval;
        char *sval;
        Image *img;
    } u;
} Value;

// --- END NEW DEFINITIONS ---


// Function declarations
void eval_program(Ast *prog);
void eval_stmt(Ast *stmt);
Value eval_expr(Ast *expr); // <-- Return type changed
// ... all other prototypes ...
void env_shutdown();
// New environment and error functions
void env_set(const char *name, Value val);
Value env_get(const char *name);
void runtime_error(const char *format, ...);
void free_value(Value val);

#endif
