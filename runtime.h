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
Image *adjust_contrast(Image *img, int amount, int direction);
Image *apply_threshold(Image *img, int threshold, int direction);
Image *convolve_image(Image *img, float kernel[3][3]);
Image *sharpen_image(Image *img, int amount, int direction);
Image *blend_images(Image *img1, Image *img2, float alpha);
Image *mask_image(Image *img, Image *mask);
Image *resize_image_nearest(Image *img, int new_w, int new_h);
Image *scale_image_factor(Image *img, float factor);
Image *rotate_image_90(Image *img, int direction) ;

void print_string_escaped(const char *s);

#endif
