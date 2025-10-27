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


Image *grayscale_image(Image *img);
Image *invert_image(Image *img);
Image *flip_image_along_X(Image *img);
Image *flip_image_along_Y(Image *img);
Image* run_canny(Image *img, float sigma, unsigned char low_thresh, unsigned char high_thresh);
Image *adjust_brightness(Image *img, int bias, int direction);

#endif
