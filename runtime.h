#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdlib.h>
#include <stdint.h>

typedef struct {
    int width, height, channels;
    unsigned char *data;
} Image;

// runtime ops
Image *load_image(const char *filename);
void save_image(const char *filename, Image *img);
Image *crop_image(Image *img, int x, int y, int w, int h);
Image *blur_image(Image *img, int radius);
void free_image(Image *img);

#endif
