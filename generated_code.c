/* Generated C code from IML Compiler */

#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>

int main() {

    // Variable Declarations (Hoisted from IML script)
    Image* img2 = NULL;
    Image* img = NULL;

    // Program Logic (Assignments and Expressions)
char* __temp_0 = "input2.png";Image* __temp_1 = load_image(__temp_0);int __temp_2 = 200;int __temp_3 = 1;Image* __temp_4 = apply_threshold(__temp_1, __temp_2, __temp_3);
    img = __temp_4;char* __temp_5 = "input3.png";Image* __temp_6 = load_image(__temp_5);Image* __temp_7 = mask_image(__temp_6, img);
    img2 = __temp_7;char* __temp_8 = "output.jpg";save_image(__temp_8, img2);
    char* __temp_10 = "output2.jpg";save_image(__temp_10, img);
    

    return 0;
}
