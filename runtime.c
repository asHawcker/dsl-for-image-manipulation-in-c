#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "runtime.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Image *load_image(const char *filename) {
    if (!filename) {
        fprintf(stderr, "Error: NULL filename in load_image\n");
        return NULL;
    }
    Image *img = malloc(sizeof(Image));
    if (!img) {
        fprintf(stderr, "Error: Memory allocation failed in load_image\n");
        return NULL;
    }
    // Force 3 channels (RGB) to ensure color
    img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 3);
    if (!img->data) {
        fprintf(stderr, "Error: Failed to load image %s\n", filename);
        free(img);
        return NULL;
    }
    img->channels = 3; // Explicitly set to RGB
    return img;
}

void save_image(const char *filename, Image *img) {
    if (!filename || !img || !img->data) {
        fprintf(stderr, "Error: Invalid save_image parameters (filename=%p, img=%p, data=%p)\n",
                (void*)filename, (void*)img, img ? (void*)img->data : NULL);
        return;
    }
    // Explicitly use 3 channels for PNG
    stbi_write_png(filename, img->width, img->height, 3, img->data, img->width * 3);
}

Image *crop_image(Image *img, int x, int y, int w, int h) {
    if (!img || !img->data || w <= 0 || h <= 0 || x < 0 || y < 0) {
        fprintf(stderr, "Error: Invalid crop parameters (img=%p, data=%p, x=%d, y=%d, w=%d, h=%d)\n",
                (void*)img, img ? (void*)img->data : NULL, x, y, w, h);
        return NULL;
    }
    if (x + w > img->width || y + h > img->height) {
        fprintf(stderr, "Error: Crop out of bounds (img: %dx%d, crop: x=%d, y=%d, w=%d, h=%d)\n",
                img->width, img->height, x, y, w, h);
        return NULL;
    }
    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in crop_image\n");
        return NULL;
    }
    out->width = w;
    out->height = h;
    out->channels = 3;  // Force RGB
    size_t row_size = w * out->channels;
    out->data = malloc(h * row_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for crop data\n");
        free(out);
        return NULL;
    }
    // Initialize output to zero to avoid garbage
    memset(out->data, 0, h * row_size);
    for (int i = 0; i < h; i++) {
        size_t src_offset = ((y + i) * img->width + x) * img->channels;
        size_t dst_offset = i * row_size;
        memcpy(out->data + dst_offset, img->data + src_offset, row_size);
    }
    return out;
}

// Simple box blur
Image *blur_image(Image *img, int radius) {
    if (!img || !img->data || radius < 1) {
        fprintf(stderr, "Error: Invalid blur parameters (img=%p, data=%p, radius=%d)\n",
                (void*)img, img ? (void*)img->data : NULL, radius);
        return NULL;
    }
    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in blur_image\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3;  // Force RGB
    size_t data_size = img->width * img->height * out->channels;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for blur data\n");
        free(out);
        return NULL;
    }
    int w = img->width, h = img->height, c = out->channels;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            long long sum[3] = {0};  // long long for overflow, 3 for RGB
            int count = 0;
            for (int dy = -radius; dy <= radius; dy++) {
                int yy = y + dy;
                if (yy < 0 || yy >= h) continue;
                for (int dx = -radius; dx <= radius; dx++) {
                    int xx = x + dx;
                    if (xx < 0 || xx >= w) continue;
                    unsigned char *p = img->data + (yy * w + xx) * c;
                    for (int ch = 0; ch < c; ch++) sum[ch] += p[ch];
                    count++;
                }
            }
            unsigned char *q = out->data + (y * w + x) * c;
            for (int ch = 0; ch < c; ch++) q[ch] = (unsigned char)(sum[ch] / count);
        }
    }
    return out;
}

void free_image(Image *img) {
    if (!img) return;
    if (img->data) stbi_image_free(img->data);
    free(img);
}
