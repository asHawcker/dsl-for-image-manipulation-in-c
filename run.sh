#!/bin/bash

# run.sh: Build and run the IML image manipulation project
# Assumes all source files (parser.y, lexer.l, ast.c, runtime.c, main.c, 
# codegen.c, eval.c, ast.h, runtime.h, codegen.h, etc.) are in the current directory.
# Requires: bison, flex, gcc (with -lm for math lib), and a sample input.png for testing.
# Usage: ./run.sh [script.iml]

# Default script if none provided
DEFAULT_SCRIPT="script.iml"
if [ ! -f "$DEFAULT_SCRIPT" ]; then
    cat > "$DEFAULT_SCRIPT" << 'EOF'
image img = load("input.png");
image img_cropped = img |> crop(50, 50, 300, 300);
save("output.png", img_cropped);
EOF
    echo "Created default $DEFAULT_SCRIPT. Ensure 'input.png' exists for testing."
fi

SCRIPT="${1:-$DEFAULT_SCRIPT}"

# Build
echo "Building IML Compiler..."
bison -d parser.y
flex lexer.l
# NEW: Added optimize.c to the build
gcc -o iml parser.tab.c lex.yy.c ast.c codegen.c eval.c main.c runtime.c optimize.c -lm -Wall 

if [ $? -ne 0 ]; then
    echo "Build failed! (Failed to create the 'iml' compiler executable)"
    exit 1
fi

# Run
# The 'iml' executable will now run, generate C code, compile the C code, and execute the result.
echo "Running IML Compiler on $SCRIPT..."
./iml "$SCRIPT"

if [ $? -eq 0 ]; then
    echo "Success! The generated code has been compiled and executed. Check for 'a.out' and its output files."
else
    echo "Compiler (or compiled program) failed!"
    exit 1
fi

# Cleanup generated files (optional; comment out if wanted)
# rm -f parser.tab.c parser.tab.h lex.yy.c iml generated_code.c a.out