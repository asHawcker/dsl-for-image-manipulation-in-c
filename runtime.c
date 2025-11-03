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


/**
 * @brief Adjusts the contrast of an image.
 *
 * Implements contrast adjustment using the formula:
 * New = Factor * (Old - 128) + 128
 * The factor is derived from the amount and direction.
 *
 * @param img The source Image.
 * @param amount The amount to adjust (0-100).
 * @param direction 1 to increase contrast, 0 to reduce contrast.
 * @return A new, contrast-adjusted Image, or NULL on failure.
 */

Image *adjust_contrast(Image *img, int amount, int direction) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in adjust_contrast\n");
        return NULL;
    }
    
    if (amount < 0) amount = 0;
    if (amount > 100) amount = 100;

    // Allocate new image space
    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in adjust_contrast\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3; // Force 3 channels
    size_t data_size = (size_t)img->width * img->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for contrast data\n");
        free(out);
        return NULL;
    }

    // Calculate the contrast factor
    float factor;
    if (direction == 1) {
        factor = 1.0f + (float)amount / 100.0f;
    } else {
        factor = 1.0f - (float)amount / 100.0f;
    }

    // Process R, G, and B channels
    for (size_t i = 0; i < data_size; i++) {
        float old_val = (float)img->data[i];
        
        //contrast formula
        float new_val_f = (factor * (old_val - 128.0f)) + 128.0f;
        // Clamp the value to the valid 0-255 range
        if (new_val_f < 0.0f) {
            out->data[i] = 0;
        } else if (new_val_f > 255.0f) {
            out->data[i] = 255;
        } else {
            out->data[i] = (unsigned char)new_val_f;
        }
    }

    return out;
}

/**
 * @brief Applies a binary threshold to an image.
 *
 * The function first converts the image to grayscale.
 * Then, it sets pixel values to 0 (black) or 255 (white)
 * based on the threshold and direction.
 *
 * @param img The source Image.
 * @param threshold The threshold value (0-255).
 * @param direction 1: (val > thresh) ? 255 : 0.  0: (val > thresh) ? 0 : 255.
 * @return A new, binary (but 3-channel) Image, or NULL on failure.
 */
Image *apply_threshold(Image *img, int threshold, int direction) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in apply_threshold\n");
        return NULL;
    }

    if (threshold < 0) threshold = 0;
    if (threshold > 255) threshold = 255;

    Image *gray_img = grayscale_image(img);
    if (!gray_img) {
        fprintf(stderr, "Error: Grayscale conversion failed in apply_threshold\n");
        return NULL;
    }

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in apply_threshold\n");
        free_image(gray_img);
        return NULL;
    }
    out->width = gray_img->width;
    out->height = gray_img->height;
    out->channels = 3;
    size_t data_size = (size_t)out->width * out->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for threshold data\n");
        free_image(gray_img);
        free(out);
        return NULL;
    }

    unsigned char *src_data = gray_img->data;
    unsigned char *dst_data = out->data;

    for (size_t i = 0; i < data_size; i += 3) {
        unsigned char value = src_data[i];
        unsigned char out_val;

        if (direction == 1) {
            out_val = (value > threshold) ? 255 : 0;
        } else {
            out_val = (value > threshold) ? 0 : 255;
        }

        dst_data[i] = out_val;
        dst_data[i + 1] = out_val;
        dst_data[i + 2] = out_val;
    }

    free_image(gray_img);

    return out;
}

/**
 * @brief Helper to clamp a float value to the 0-255 byte range.
 */
static inline unsigned char clamp_pixel(float v) {
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}

/**
 * @brief Applies a 3x3 convolution kernel to an image.
 *
 * @param img The source Image.
 * @param kernel A 3x3 float kernel.
 * @return A new, convolved Image, or NULL on failure.
 */
Image *convolve_image(Image *img, float kernel[3][3]) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in convolve_image\n");
        return NULL;
    }

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in convolve_image\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3;
    size_t data_size = (size_t)out->width * out->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for convolve_image data\n");
        free(out);
        return NULL;
    }

    int w = img->width, h = img->height, c = img->channels;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;

            // for borders
            if (y == 0 || y == h - 1 || x == 0 || x == w - 1) {
                unsigned char *p = img->data + (y * w + x) * c;
                sum_r = p[0];
                sum_g = p[1];
                sum_b = p[2];
            } else {
                // Apply 3x3 kernel
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        unsigned char *p = img->data + ((y + ky) * w + (x + kx)) * c;
                        float kval = kernel[ky + 1][kx + 1];
                        sum_r += p[0] * kval;
                        sum_g += p[1] * kval;
                        sum_b += p[2] * kval;
                    }
                }
            }
            unsigned char *q = out->data + (y * w + x) * c;
            q[0] = clamp_pixel(sum_r);
            q[1] = clamp_pixel(sum_g);
            q[2] = clamp_pixel(sum_b);
        }
    }
    return out;
}

/**
 * @brief Sharpens or softens an image.
 *
 * direction = 1 (Sharpen): Applies a 3x3 sharpen kernel. 'amount' controls
 * the strength (recommended 1-10).
 * direction = 0 (Soften):  Applies a box blur. 'amount' is used as the
 * blur radius.
 *
 * @param img The source Image.
 * @param amount The strength (for sharpen) or radius (for soften).
 * @param direction 1 to sharpen, 0 to soften/blur.
 * @return A new, processed Image, or NULL on failure.
 */
Image *sharpen_image(Image *img, int amount, int direction) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in sharpen_image\n");
        return NULL;
    }

    if (direction == 0) {
        if (amount < 1) amount = 1; 
        return blur_image(img, amount);
    }

    float k = (float)amount / 10.0f; 
    float kernel[3][3] = {
        { 0.0f, -k, 0.0f },
        { -k, 1.0f + 4.0f * k, -k },
        { 0.0f, -k, 0.0f }
    };

    return convolve_image(img, kernel);
}


/**
 * @brief Blends two images together using a specified alpha.
 *
 * The images must have the same dimensions.
 * The blend formula is: out = img1 * (1.0 - alpha) + img2 * alpha
 *
 * @param img1 The first source Image (visible at alpha=0.0).
 * @param img2 The second source Image (visible at alpha=1.0).
 * @param alpha The blend factor (0.0 to 1.0).
 * @return A new, blended Image, or NULL on failure.
 */
Image *blend_images(Image *img1, Image *img2, float alpha) {
    if (!img1 || !img1->data || !img2 || !img2->data) {
        fprintf(stderr, "Error: Invalid image(s) in blend_images\n");
        return NULL;
    }

    if (img1->width != img2->width || img1->height != img2->height) {
        fprintf(stderr, "Error: Image dimensions must match in blend_images (%dx%d vs %dx%d)\n",
                img1->width, img1->height, img2->width, img2->height);
        return NULL;
    }

    // Clamp alpha
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in blend_images\n");
        return NULL;
    }
    out->width = img1->width;
    out->height = img1->height;
    out->channels = 3;
    size_t data_size = (size_t)out->width * out->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for blend_images data\n");
        free(out);
        return NULL;
    }

    float alpha_neg = 1.0f - alpha;
    unsigned char *p1 = img1->data;
    unsigned char *p2 = img2->data;
    unsigned char *q = out->data;

    for (size_t i = 0; i < data_size; i++) {
        // Apply blend formula to each component
        float val = (p1[i] * alpha_neg) + (p2[i] * alpha);
        q[i] = clamp_pixel(val);
    }

    return out;
}


/**
 * @brief Applies a binary mask to an image.
 *
 * The images must have the same dimensions.
 * The output pixel will be:
 * - Copied from img if the corresponding mask pixel is "on" (non-black).
 * - Black if the corresponding mask pixel is "off" (black).
 *
 * @param img The source Image to be masked.
 * @param mask The binary mask Image (assumed to be 3-channel grayscale, 0 or 255).
 * @return A new, masked Image, or NULL on failure.
 */
Image *mask_image(Image *img, Image *mask) {
    if (!img || !img->data || !mask || !mask->data) {
        fprintf(stderr, "Error: Invalid image(s) in mask_image\n");
        return NULL;
    }

    if (img->width != mask->width || img->height != mask->height) {
        fprintf(stderr, "Error: Image dimensions must match in mask_image (%dx%d vs %dx%d)\n",
                img->width, img->height, mask->width, mask->height);
        return NULL;
    }

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in mask_image\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3;
    size_t data_size = (size_t)out->width * out->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for mask_image data\n");
        free(out);
        return NULL;
    }

    unsigned char *s_data = img->data;
    unsigned char *m_data = mask->data;
    unsigned char *d_data = out->data;

    size_t num_pixels = (size_t)out->width * out->height;
    for (size_t i = 0; i < num_pixels; i++) {
        unsigned char *s_ptr = s_data + i * 3;
        unsigned char *m_ptr = m_data + i * 3;
        unsigned char *d_ptr = d_data + i * 3;

        if (m_ptr[0] > 0) {
            memcpy(d_ptr, s_ptr, 3);
        } else {
            d_ptr[0] = 0;
            d_ptr[1] = 0;
            d_ptr[2] = 0;
        }
    }

    return out;
}


/**
 * @brief Resizes an image using the nearest-neighbor algorithm.
 *
 * @param img The source Image to resize.
 * @param new_w The target width.
 * @param new_h The target height.
 * @return A new, resized Image, or NULL on failure.
 */
Image *resize_image_nearest(Image *img, int new_w, int new_h) {
    if (!img || !img->data || new_w <= 0 || new_h <= 0) {
        fprintf(stderr, "Error: Invalid parameters in resize_image_nearest (img=%p, data=%p, w=%d, h=%d)\n",
                (void*)img, img ? (void*)img->data : NULL, new_w, new_h);
        return NULL;
    }

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in resize_image_nearest\n");
        return NULL;
    }
    out->width = new_w;
    out->height = new_h;
    out->channels = 3;
    size_t data_size = (size_t)new_w * new_h * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for resize_image_nearest data\n");
        free(out);
        return NULL;
    }

    int old_w = img->width;
    int old_h = img->height;
    unsigned char *s_data = img->data;
    unsigned char *d_data = out->data;

    float x_ratio = old_w / (float)new_w;
    float y_ratio = old_h / (float)new_h;

    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            int src_x = (int)(x * x_ratio);
            int src_y = (int)(y * y_ratio);

            unsigned char *src_pixel = s_data + (src_y * old_w + src_x) * 3;
            unsigned char *dst_pixel = d_data + (y * new_w + x) * 3;

            memcpy(dst_pixel, src_pixel, 3);
        }
    }

    return out;
}

Image *scale_image_factor(Image *img, float factor) {
    if (!img || !img->data || factor <= 0.0f) {
        fprintf(stderr, "Error: Invalid parameters for scale\n");
        return NULL;
    }
    
    int w_out = (int)(img->width * factor);
    int h_out = (int)(img->height * factor);
    
    if (w_out <= 0 || h_out <= 0) {
         fprintf(stderr, "Error: Scale factor results in zero or negative size\n");
         return NULL;
    }
    
    return resize_image_nearest(img, w_out, h_out);
}

/**
 * @brief Rotates an image by 90 degrees left or right.
 *
 * This is much faster than the general-purpose rotate.
 * Note: The output image will have its width and height swapped.
 *
 * @param img The source image.
 * @param direction 1 for 90-degrees clockwise (right), -1 for 90-degrees counter-clockwise (left).
 * @return A new, rotated Image, or NULL on failure.
 */
Image *rotate_image_90(Image *img, int direction) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in rotate_image_90\n");
        return NULL;
    }
    if (direction != 1 && direction != -1) {
        fprintf(stderr, "Error: Invalid direction for rotate_image_90 (must be 1 or -1)\n");
        return NULL;
    }

    int w_in = img->width;
    int h_in = img->height;
    int c_in = img->channels;

    int w_out = h_in;
    int h_out = w_in;

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed for Image struct (rotate_90)\n");
        return NULL;
    }
    out->width = w_out;
    out->height = h_out;
    out->channels = 3;
    size_t data_size = w_out * h_out * out->channels;
    
    out->data = malloc(data_size);
    if (!out->data) {
         fprintf(stderr, "Error: Memory allocation failed for image data (rotate_90)\n");
         free(out);
         return NULL;
    }
    

    if (c_in < 3) {
         fprintf(stderr, "Error: rotate_90 input must have at least 3 channels\n");
         free(out->data);
         free(out);
         return NULL;
    }

    for (int y_out = 0; y_out < h_out; y_out++) {
        for (int x_out = 0; x_out < w_out; x_out++) {
            int x_src, y_src;

            if (direction == 1) { 
                x_src = y_out;
                y_src = h_in - 1 - x_out;
            } else { 
                x_src = w_in - 1 - y_out;
                y_src = x_out;
            }

            unsigned char *p = img->data + (y_src * w_in + x_src) * c_in;
            unsigned char *q = out->data + (y_out * w_out + x_out) * 3;
            
            q[0] = p[0];
            q[1] = p[1];
            q[2] = p[2];
        }
    }
    
    return out;
}

// Helper function to print a string while interpreting basic escape sequences
void print_string_escaped(const char *s) {
    if (!s) return;

    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\\') {
            i++;
            switch (s[i]) {
                case 'n':
                    putchar('\n');
                    break;
                case 't':
                    putchar('\t');
                    break;
                case '\\':
                    putchar('\\');
                    break;
                case '"':
                    putchar('"'); 
                    break;
                case '\0':
                    putchar('\\');
                    return;
                default:
                    putchar('\\');
                    putchar(s[i]);
                    break;
            }
        } else {
            putchar(s[i]);
        }
    }
}