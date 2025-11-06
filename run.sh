#!/bin/bash
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

echo "Building IML Compiler..."
bison -d parser.y
flex lexer.l
gcc -o iml parser.tab.c lex.yy.c ast.c codegen.c eval.c main.c runtime.c optimize.c -lm -Wall 

if [ $? -ne 0 ]; then
    echo "Build failed! (Failed to create the 'iml' compiler executable)"
    exit 1
fi

echo "Running IML Compiler on $SCRIPT..."
./iml "$SCRIPT"

if [ $? -eq 0 ]; then
    echo "Success! The generated code has been compiled and executed. Check for 'a.out' and its output files."
else
    echo "Compiler (or compiled program) failed!"
    exit 1
fi
