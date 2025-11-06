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
    img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 3);
    if (!img->data) {
        fprintf(stderr, "Error: Failed to load image %s\n", filename);
        free(img);
        return NULL;
    }
    img->channels = 3;
    return img;
}

void save_image(const char *filename, Image *img) {
    if (!filename || !img || !img->data) {
        fprintf(stderr, "Error: Invalid save_image parameters (filename=%p, img=%p, data=%p)\n",
                (void*)filename, (void*)img, img ? (void*)img->data : NULL);
        return;
    }
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
    out->channels = 3; 
    size_t row_size = w * out->channels;
    out->data = malloc(h * row_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for crop data\n");
        free(out);
        return NULL;
    }
    memset(out->data, 0, h * row_size);
    for (int i = 0; i < h; i++) {
        size_t src_offset = ((y + i) * img->width + x) * img->channels;
        size_t dst_offset = i * row_size;
        memcpy(out->data + dst_offset, img->data + src_offset, row_size);
    }
    return out;
}

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

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in grayscale_image\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3; 
    size_t data_size = img->width * img->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for grayscale data\n");
        free(out);
        return NULL;
    }

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            unsigned char *p = img->data + (y * img->width + x) * 3;
            unsigned char *q = out->data + (y * img->width + x) * 3;

            int r = p[0];
            int g = p[1];
            int b = p[2];
            unsigned char gray = (unsigned char)((299 * r + 587 * g + 114 * b) / 1000);

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

    for (int y = 0; y < img->height; y++) {
        int src_y = (img->height - 1) - y; 
        int dst_y = y;                     
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
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int src_x = (img->width - 1) - x; 
            int dst_x = x;                    

            unsigned char *src_ptr = img->data + (y * img->width + src_x) * 3;
            unsigned char *dst_ptr = out->data + (y * img->width + dst_x) * 3;

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

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in adjust_brightness\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3; 
    size_t data_size = (size_t)img->width * img->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for brightness data\n");
        free(out);
        return NULL;
    }

    int final_bias = (direction == 1) ? bias : -bias;

    for (size_t i = 0; i < data_size; i++) {
        int new_val = (int)img->data[i] + final_bias;

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




Image *adjust_contrast(Image *img, int amount, int direction) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in adjust_contrast\n");
        return NULL;
    }
    
    if (amount < 0) amount = 0;
    if (amount > 100) amount = 100;

    Image *out = malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed in adjust_contrast\n");
        return NULL;
    }
    out->width = img->width;
    out->height = img->height;
    out->channels = 3; 
    size_t data_size = (size_t)img->width * img->height * 3;
    out->data = malloc(data_size);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for contrast data\n");
        free(out);
        return NULL;
    }

    float factor;
    if (direction == 1) {
        factor = 1.0f + (float)amount / 100.0f;
    } else {
        factor = 1.0f - (float)amount / 100.0f;
    }

    for (size_t i = 0; i < data_size; i++) {
        float old_val = (float)img->data[i];
        
        float new_val_f = (factor * (old_val - 128.0f)) + 128.0f;
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

static inline unsigned char clamp_pixel(float v) {
    if (v < 0.0f) return 0;
    if (v > 255.0f) return 255;
    return (unsigned char)v;
}


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

            if (y == 0 || y == h - 1 || x == 0 || x == w - 1) {
                unsigned char *p = img->data + (y * w + x) * c;
                sum_r = p[0];
                sum_g = p[1];
                sum_b = p[2];
            } else {
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
        float val = (p1[i] * alpha_neg) + (p2[i] * alpha);
        q[i] = clamp_pixel(val);
    }

    return out;
}


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