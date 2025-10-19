# IML: Image Manipulation Language

IML is a simple scripting language for image processing, built with C using Bison (parser), Flex (lexer), and the STB image library. It supports operations like loading, cropping, blurring, and saving images, with a pipeline syntax (`|>`) for chaining transformations.

## Features
- **Load/Save Images**: Read and write PNG/JPG images using `load` and `save`.
- **Crop**: Extract rectangular regions with `crop(x, y, width, height)`.
- **Blur**: Apply a box blur with `blur(radius)`.
- **Pipeline Syntax**: Chain operations (e.g., `load("input.png") |> crop(50,50,300,300)`).
- **Error Handling**: Logs invalid crop bounds or memory issues to prevent crashes or incorrect outputs (e.g., gray images).
- **AST Debugging**: Use `--dump-ast` to inspect the Abstract Syntax Tree.

## Prerequisites
- **Tools**:
  - `gcc` (C compiler)
  - `bison` (parser generator)
  - `flex` (lexer generator)
- **Libraries**:
  - STB image libraries: `stb_image.h` and `stb_image_write.h` (download from https://github.com/nothings/stb).
- **Operating System**:
  - Linux/macOS (recommended) or Windows with WSL/MinGW/Cygwin.
- **Input Image**:
  - A colorful RGB PNG/JPG named `input.png` (e.g., at least 400x400 pixels for default scripts).

### Installation (Linux/macOS)
```bash
# Install tools
sudo apt-get update
sudo apt-get install gcc bison flex
# On macOS (with Homebrew)
brew install gcc bison flex
```
- Place `stb_image.h` and `stb_image_write.h` in the project directory.

## Project Structure
- **Source Files**:
  - `parser.y`: Bison grammar for IML syntax.
  - `lexer.l`: Flex lexer for tokenizing input scripts.
  - `ast.c`, `ast.h`: Abstract Syntax Tree (AST) definitions and utilities.
  - `runtime.c`, `runtime.h`: Image processing functions (load, save, crop, blur).
  - `eval.c`, `eval.h`: AST evaluation logic.
  - `main.c`: Program entry point.
  - `run.sh`: Build and run script.
- **Dependencies**:
  - `stb_image.h`, `stb_image_write.h`: For image I/O.

## Building
Use the provided `run.sh` script to build and run:
```bash
chmod +x run.sh
./run.sh
```
This:
1. Generates parser/lexer with `bison -d parser.y` and `flex lexer.l`.
2. Compiles with `gcc -o iml parser.tab.c lex.yy.c ast.c runtime.c main.c eval.c -lm -Wall`.
3. Runs the default `script.iml` with `--dump-ast`.

Alternatively, build manually:
```bash
bison -d parser.y
flex lexer.l
gcc -o iml parser.tab.c lex.yy.c ast.c runtime.c main.c eval.c -lm -Wall
```

## Usage
Run the compiled binary with an IML script:
```bash
./iml script.iml [--dump-ast]
```
- `script.iml`: Your IML script (e.g., see samples below).
- `--dump-ast`: Optional; prints the AST for debugging.
- If no script is provided, `run.sh` creates a default `script.iml` that crops `input.png`.

### Sample Scripts
Save these as `.iml` files in the project directory and run with `./run.sh filename.iml`.

#### 1. Simple Load/Save (sample1.iml)
```iml
img = load("input.png");
save("output.png", img);
```
- **Output**: `output.png` is identical to `input.png`.
- **Use**: Verify image I/O.

#### 2. Crop Only (sample2.iml)
```iml
img = load("input.png") |> crop(50, 50, 300, 300);
save("output.png", img);
```
- **Output**: `output.png` is a 300x300 crop from (50,50) of `input.png`.
- **Note**: Ensure `input.png` is at least 350x350 pixels.

#### 3. Double Crop (sample3.iml)
```iml
temp = load("input.png") |> crop(0, 0, 400, 400);
final = temp |> crop(100, 100, 200, 200);
save("double_cropped.png", final);
```
- **Output**: `double_cropped.png` is a 200x200 crop (effective offset 100,100 from original).
- **Note**: Ensure `input.png` is at least 400x400 pixels.
