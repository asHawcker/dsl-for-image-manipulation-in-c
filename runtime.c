#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image.h"
#include "include/stb_image_write.h"
#include "include/canny.h"
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

Image *grayscale_image(Image *img) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in grayscale_image\n");
        return NULL;
    }

    // Allocate new image struct
    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in grayscale_image\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3; // Keep 3 channels
    size_t data_size = img->width * img->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for grayscale data\n");
        free(out);
        return NULL;
    }

    // Process each pixel
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            // Get pointers to source and destination pixels
            unsigned char *p = img->data + (y * img->width + x) * 3;
            unsigned char *q = out->data + (y * img->width + x) * 3;

            // Use integer math for luminance calculation (avoids floats)
            // Y = (299*R + 587*G + 114*B) / 1000
            int r = p[0];
            int g = p[1];
            int b = p[2];
            unsigned char gray = (unsigned char)((299 * r + 587 * g + 114 * b) / 1000);

            // Set R, G, and B to the same grayscale value
            q[0] = gray;
            q[1] = gray;
            q[2] = gray;
        }
    }
    return out;
}

Image *invert_image(Image *img) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in invert_image\n");
        return NULL;
    }

    // Allocate new image struct
    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in invert_image\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3;
    size_t data_size = img->width * img->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for invert data\n");
        free(out);
        return NULL;
    }

    // Process each byte (R, G, and B components)
    for (size_t i = 0; i < data_size; i++) {
        out->data[i] = 255 - img->data[i];
    }
    return out;
}

Image *flip_image_along_X(Image *img) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in flip_image_vertical\n");
        return NULL;
    }

    // Allocate new image struct
    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in flip_image_vertical\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3;
    size_t row_size = (size_t)img->width * 3; // Size of one row in bytes
    out->data = malloc(img->height * row_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for flip data\n");
        free(out);
        return NULL;
    }

    // Copy rows from source to destination in reverse order
    for (int y = 0; y < img->height; y++) {
        int src_y = (img->height - 1) - y; // Source row (from bottom up)
        int dst_y = y;                     // Destination row (from top down)

        unsigned char *src_row_ptr = img->data + (src_y * row_size);
        unsigned char *dst_row_ptr = out->data + (dst_y * row_size);

        memcpy(dst_row_ptr, src_row_ptr, row_size);
    }
    return out;
}

Image *flip_image_along_Y(Image *img) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in flip_image_horizontal\n");
        return NULL;
    }

    // Allocate new image struct
    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in flip_image_horizontal\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3;
    size_t data_size = (size_t)img->width * img->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for flip data\n");
        free(out);
        return NULL;
    }

    // Process each row
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            // Find source and destination pixel locations
            int src_x = (img->width - 1) - x; // Source pixel (from right)
            int dst_x = x;                     // Destination pixel (from left)

            // Get pointers to source and destination pixels
            unsigned char *src_ptr = img->data + (y * img->width + src_x) * 3;
            unsigned char *dst_ptr = out->data + (y * img->width + dst_x) * 3;

            // Copy the 3 bytes (R, G, B)
            memcpy(dst_ptr, src_ptr, 3);
        }
    }
    return out;
}

Image* run_canny(Image *img, float sigma, unsigned char low_thresh, unsigned char high_thresh){

    return canny_edge_detector(img, sigma, low_thresh, high_thresh);
}

Image *adjust_brightness(Image *img, int bias, int direction) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in adjust_brightness\n");
        return NULL;
    }

    // Allocate new image struct
    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in adjust_brightness\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3; // Force 3 channels
    size_t data_size = (size_t)img->width * img->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for brightness data\n");
        free(out);
        return NULL;
    }

    // Determine the actual value to add (-bias or +bias)
    int final_bias = (direction == 1) ? bias : -bias;

    // Process every byte (R, G, and B channels)
    for (size_t i = 0; i < data_size; i++) {
        // Calculate new value
        int new_val = (int)img->data[i] + final_bias;

        // Clamp the value to the valid 0-255 range
        if (new_val < 0) {
            out->data[i] = 0;
        } else if (new_val > 255) {
            out->data[i] = 255;
        } else {
            out->data[i] = (unsigned char)new_val;
        }
    }

    return out;
}