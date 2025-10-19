#!/bin/bash

# run.sh: Build and run the IML image manipulation project
# Assumes all source files (parser.y, lexer.l, ast.c, runtime.c, main.c, eval.c, eval.h, ast.h, runtime.h, stb_image.h, stb_image_write.h) are in the current directory.
# Requires: bison, flex, gcc (with -lm for math lib), and a sample input.png for testing.
# Usage: ./run.sh [script.iml] [--dump-ast]
# If no script.iml provided, uses a default one.

# Default script if none provided
DEFAULT_SCRIPT="script.iml"
if [ ! -f "$DEFAULT_SCRIPT" ]; then
    cat > "$DEFAULT_SCRIPT" << 'EOF'
img = load("input.png") |> crop(50, 50, 300, 300);
save("output.png", img);
EOF
    echo "Created default $DEFAULT_SCRIPT. Ensure 'input.png' exists for testing."
fi

SCRIPT="${1:-$DEFAULT_SCRIPT}"
DUMP_AST="${2:---dump-ast}"  # Default to dump AST for verification

# Build
echo "Building IML..."
bison -d parser.y
flex lexer.l
gcc -o iml parser.tab.c lex.yy.c ast.c runtime.c main.c eval.c -lm -Wall

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Run
echo "Running $SCRIPT $DUMP_AST..."
./iml "$SCRIPT" "$DUMP_AST"

if [ $? -eq 0 ]; then
    echo "Success! Check output.png (if script saves it)."
else
    echo "Run failed!"
    exit 1
fi

# Cleanup generated files (optional; comment out if wanted)
# rm -f parser.tab.c parser.tab.h lex.yy.c iml
