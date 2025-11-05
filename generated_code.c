/* Generated C code from IML Compiler */

#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>

int main() {

    // Variable Declarations (Hoisted from IML script)
    Image* img2 = NULL;
    Image* img = NULL;

    // Program Logic

    img = apply_threshold(load_image("input2.png"), 200, 1);
    img2 = mask_image(load_image("input3.png"), img);
    save_image("output.jpg", img2);
    save_image("output2.jpg", img);


    return 0;
}
