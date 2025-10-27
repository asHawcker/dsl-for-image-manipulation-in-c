#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../runtime.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CANNY_WEAK 50
#define CANNY_STRONG 255


// static unsigned char* grayscale_to_mono(Image *img);
// static float* create_gaussian_kernel(float sigma, int *kernel_size);
// static unsigned char* gaussian_blur_mono(unsigned char *data, int w, int h, float sigma);
// static int sobel_operator(unsigned char *data, int w, int h, float **magnitude, float **direction);
// static unsigned char* non_maximum_suppression(float *magnitude, float *direction, int w, int h);
// static void hysteresis_connect(unsigned char *data, int w, int h, int y, int x);
// static void double_threshold_hysteresis(unsigned char *data, int w, int h, unsigned char low, unsigned char high);
// Image *canny_edge_detector(Image *img, float sigma, unsigned char low_thresh, unsigned char high_thresh);

// #include "canny.h"



/**
 * @brief Helper to convert 3-channel RGB image to a 1-channel grayscale buffer.
 * @param img The source 3-channel image.
 * @return A new 1-channel (unsigned char*) buffer, or NULL on failure.
 */
static unsigned char* grayscale_to_mono(Image *img) {
    if (!img || !img->data) return NULL;
    size_t data_size = (size_t)img->width * img->height;
    unsigned char *mono_data = (unsigned char*)malloc(data_size);
    if (!mono_data) {
        fprintf(stderr, "Error: Memory allocation failed in grayscale_to_mono\n");
        return NULL;
    }

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            unsigned char *p = img->data + (y * img->width + x) * 3;
            // Using standard luminance calculation (0.299*R + 0.587*G + 0.114*B)
            mono_data[y * img->width + x] = (unsigned char)(0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2]);
        }
    }
    return mono_data;
}

/**
 * @brief Helper to create a 1D Gaussian kernel.
 * @param sigma Standard deviation.
 * @param kernel_size Pointer to an int to store the resulting kernel size.
 * @return A new 1D (float*) kernel, or NULL on failure.
 */
static float* create_gaussian_kernel(float sigma, int *kernel_size) {
    int radius = (int)ceil(3 * sigma);
    *kernel_size = 2 * radius + 1;
    float *kernel = (float*)malloc(*kernel_size * sizeof(float));
    if (!kernel) {
        fprintf(stderr, "Error: Memory allocation failed for Gaussian kernel\n");
        return NULL;
    }

    float sum = 0.0;
    float two_sigma_sq = 2.0 * sigma * sigma;

    for (int i = -radius; i <= radius; i++) {
        float val = exp(-(i * i) / two_sigma_sq) / (sqrt(M_PI * two_sigma_sq));
        kernel[i + radius] = val;
        sum += val;
    }

    // Normalize kernel
    for (int i = 0; i < *kernel_size; i++) {
        kernel[i] /= sum;
    }
    return kernel;
}

/**
 * @brief Helper to apply 1D separable Gaussian blur (Horizontal + Vertical).
 * @param data The 1-channel source buffer.
 * @param w Width of the buffer.
 * @param h Height of the buffer.
 * @param sigma Standard deviation for the blur.
 * @return A new 1-channel (unsigned char*) blurred buffer, or NULL on failure.
 */
static unsigned char* gaussian_blur_mono(unsigned char *data, int w, int h, float sigma) {
    int kernel_size;
    float *kernel = create_gaussian_kernel(sigma, &kernel_size);
    if (!kernel) return NULL;
    int radius = kernel_size / 2;

    size_t data_size = (size_t)w * h;
    unsigned char *temp_data = (unsigned char *)malloc(data_size);
    unsigned char *out_data = (unsigned char *)malloc(data_size);
    if (!temp_data || !out_data) {
        fprintf(stderr, "Error: Memory allocation failed for Gaussian blur buffers\n");
        free(kernel);
        free(temp_data);
        free(out_data);
        return NULL;
    }

    // Horizontal pass (from data -> temp_data)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float sum = 0.0;
            for (int k = -radius; k <= radius; k++) {
                int kx = x + k;
                // Handle borders by clamping
                if (kx < 0) kx = 0;
                if (kx >= w) kx = w - 1;
                sum += data[y * w + kx] * kernel[k + radius];
            }
            temp_data[y * w + x] = (unsigned char)sum;
        }
    }

    // Vertical pass (from temp_data -> out_data)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float sum = 0.0;
            for (int k = -radius; k <= radius; k++) {
                int ky = y + k;
                // Handle borders by clamping
                if (ky < 0) ky = 0;
                if (ky >= h) ky = h - 1;
                sum += temp_data[ky * w + x] * kernel[k + radius];
            }
            out_data[y * w + x] = (unsigned char)sum;
        }
    }

    free(kernel);
    free(temp_data);
    return out_data;
}

/**
 * @brief Helper for Sobel operator to find gradient magnitude and direction.
 * @param data The 1-channel blurred source buffer.
 * @param w Width.
 * @param h Height.
 * @param magnitude Output pointer for gradient magnitude (float*).
 * @param direction Output pointer for gradient direction (float*).
 * @return 1 on success, 0 on failure.
 */
static int sobel_operator(unsigned char *data, int w, int h, float **magnitude, float **direction) {
    *magnitude = (float*)calloc((size_t)w * h, sizeof(float));
    *direction = (float*)calloc((size_t)w * h, sizeof(float));
    if (!*magnitude || !*direction) {
        fprintf(stderr, "Error: Memory allocation failed for Sobel buffers\n");
        return 0;
    }

    int sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int sobel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

    for (int y = 1; y < h - 1; y++) { // Skip borders
        for (int x = 1; x < w - 1; x++) {
            float gx = 0, gy = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    gx += data[(y + dy) * w + (x + dx)] * sobel_x[dy + 1][dx + 1];
                    gy += data[(y + dy) * w + (x + dx)] * sobel_y[dy + 1][dx + 1];
                }
            }
            size_t idx = y * w + x;
            (*magnitude)[idx] = sqrt(gx * gx + gy * gy);
            (*direction)[idx] = atan2(gy, gx); // Angle in radians
        }
    }
    return 1;
}

/**
 * @brief Helper for non-maximum suppression to thin edges.
 * @param magnitude Gradient magnitude buffer.
 * @param direction Gradient direction buffer.
 * @param w Width.
 * @param h Height.
 * @return A new 1-channel (unsigned char*) buffer with thinned edges, or NULL on failure.
 */
static unsigned char* non_maximum_suppression(float *magnitude, float *direction, int w, int h) {
    unsigned char *output = (unsigned char*)calloc((size_t)w * h, sizeof(unsigned char));
    if (!output) {
        fprintf(stderr, "Error: Memory allocation failed for NMS buffer\n");
        return NULL;
    }

    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            size_t idx = y * w + x;
            float mag = magnitude[idx];
            if (mag == 0) continue; // Not an edge

            float angle = direction[idx] * 180.0 / M_PI; // Convert to degrees
            if (angle < 0) angle += 180;

            float mag1 = 0, mag2 = 0;

            // --- Find neighbors in the gradient direction ---
            // Angle 0 or 180 (horizontal edge)
            if ((angle >= 0 && angle < 22.5) || (angle >= 157.5 && angle <= 180)) {
                mag1 = magnitude[idx - 1]; // Left
                mag2 = magnitude[idx + 1]; // Right
            }
            // Angle 45 (diagonal)
            else if (angle >= 22.5 && angle < 67.5) {
                mag1 = magnitude[idx - w + 1]; // Top-right
                mag2 = magnitude[idx + w - 1]; // Bottom-left
            }
            // Angle 90 (vertical edge)
            else if (angle >= 67.5 && angle < 112.5) {
                mag1 = magnitude[idx - w]; // Top
                mag2 = magnitude[idx + w]; // Bottom
            }
            // Angle 135 (diagonal)
            else if (angle >= 112.5 && angle < 157.5) {
                mag1 = magnitude[idx - w - 1]; // Top-left
                mag2 = magnitude[idx + w + 1]; // Bottom-right
            }

            // Suppress if not a local maximum
            if (mag >= mag1 && mag >= mag2) {
                // Clamp magnitude to 255
                output[idx] = (mag > 255) ? 255 : (unsigned char)mag;
            }
        }
    }
    return output;
}

/**
 * @brief Helper: Recursive part of hysteresis for edge tracking.
 * @param data The 1-channel NMS buffer (modified in-place).
 * @param w Width.
 * @param h Height.
 * @param y Current y-coordinate.
 * @param x Current x-coordinate.
 */
static void hysteresis_connect(unsigned char *data, int w, int h, int y, int x) {
    // Check bounds and if already visited or not a weak edge
    if (y < 0 || y >= h || x < 0 || x >= w || data[y * w + x] != CANNY_WEAK) {
        return;
    }
    
    data[y * w + x] = CANNY_STRONG; // Promote weak edge to strong

    // Recurse on 8 neighbors
    hysteresis_connect(data, w, h, y - 1, x - 1);
    hysteresis_connect(data, w, h, y - 1, x);
    hysteresis_connect(data, w, h, y - 1, x + 1);
    hysteresis_connect(data, w, h, y, x - 1);
    hysteresis_connect(data, w, h, y, x + 1);
    hysteresis_connect(data, w, h, y + 1, x - 1);
    hysteresis_connect(data, w, h, y + 1, x);
    hysteresis_connect(data, w, h, y + 1, x + 1);
}

/**
 * @brief Helper: Applies double thresholding and hysteresis.
 * @param data The 1-channel NMS buffer (modified in-place).
 * @param w Width.
 * @param h Height.
 * @param low Lower threshold.
 * @param high Upper threshold.
 */
static void double_threshold_hysteresis(unsigned char *data, int w, int h, unsigned char low, unsigned char high) {
    size_t data_size = (size_t)w * h;

    // 1. Apply thresholds: classify pixels as STRONG, WEAK, or 0
    for (size_t i = 0; i < data_size; i++) {
        if (data[i] >= high) {
            data[i] = CANNY_STRONG;
        } else if (data[i] >= low) {
            data[i] = CANNY_WEAK;
        } else {
            data[i] = 0;
        }
    }

    // 2. Hysteresis: connect weak edges to strong edges
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (data[y * w + x] == CANNY_STRONG) {
                // This strong edge will recursively find all connected weak edges
                hysteresis_connect(data, w, h, y, x);
            }
        }
    }

    // 3. Clean up: remove any remaining weak edges that weren't connected
    for (size_t i = 0; i < data_size; i++) {
        if (data[i] == CANNY_WEAK) {
            data[i] = 0;
        }
    }
}

/**
 * @brief Applies the Canny edge detection algorithm.
 *
 * This function performs the complete Canny algorithm:
 * 1. Grayscale conversion
 * 2. Gaussian blur
 * 3. Sobel gradient calculation
 * 4. Non-maximum suppression
 * 5. Double thresholding and hysteresis
 *
 * @param img The source Image.
 * @param sigma The standard deviation (sigma) for the Gaussian blur. (e.g., 1.4)
 * @param low_thresh The lower threshold for hysteresis. (e.g., 20)
 * @param high_thresh The upper threshold for hysteresis. (e.g., 50)
 * @return A new, 3-channel Image showing the edges, or NULL on failure.
 */
Image *canny_edge_detector(Image *img, float sigma, unsigned char low_thresh, unsigned char high_thresh) {
    if (!img || !img->data) {
        fprintf(stderr, "Error: Invalid image in canny_edge_detector\n");
        return NULL;
    }
    if (low_thresh > high_thresh) {
        fprintf(stderr, "Error: Canny low threshold cannot be greater than high threshold\n");
        return NULL;
    }

    int w = img->width;
    int h = img->height;

    // --- Step 1: Grayscale ---
    unsigned char *mono_data = grayscale_to_mono(img);
    if (!mono_data) {
        fprintf(stderr, "Error: Canny failed at grayscale step\n");
        return NULL;
    }

    // --- Step 2: Gaussian Blur ---
    unsigned char *blur_data = gaussian_blur_mono(mono_data, w, h, sigma);
    free(mono_data); // Done with original mono
    if (!blur_data) {
        fprintf(stderr, "Error: Canny failed at Gaussian blur step\n");
        return NULL;
    }

    // --- Step 3: Sobel Operator ---
    float *magnitude = NULL, *direction = NULL;
    if (!sobel_operator(blur_data, w, h, &magnitude, &direction)) {
        fprintf(stderr, "Error: Canny failed at Sobel operator step\n");
        free(blur_data);
        free(magnitude);
        free(direction);
        return NULL;
    }
    free(blur_data); // Done with blurred data

    // --- Step 4: Non-Maximum Suppression ---
    unsigned char *nms_data = non_maximum_suppression(magnitude, direction, w, h);
    free(magnitude); // Done with gradient data
    free(direction);
    if (!nms_data) {
        fprintf(stderr, "Error: Canny failed at Non-Maximum Suppression step\n");
        return NULL;
    }

    // --- Step 5: Double Thresholding and Hysteresis ---
    // This step modifies nms_data in-place
    double_threshold_hysteresis(nms_data, w, h, low_thresh, high_thresh);

    // --- Step 6: Convert final 1-channel edge map back to 3-channel RGB image ---
    Image *out = (Image*)malloc(sizeof(Image));
    if (!out) {
        fprintf(stderr, "Error: Memory allocation failed for Canny output image\n");
        free(nms_data);
        return NULL;
    }
    out->width = w;
    out->height = h;
    out->channels = 3;
    out->data = (unsigned char*)malloc((size_t)w * h * 3);
    if (!out->data) {
        fprintf(stderr, "Error: Memory allocation failed for Canny output data\n");
        free(nms_data);
        free(out);
        return NULL;
    }

    // Populate output image data (R=G=B=edge_value)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned char val = nms_data[y * w + x];
            unsigned char *p = out->data + (y * w + x) * 3;
            p[0] = val; // R
            p[1] = val; // G
            p[2] = val; // B
        }
    }

    free(nms_data); // Free the final 1-channel buffer
    return out;
}