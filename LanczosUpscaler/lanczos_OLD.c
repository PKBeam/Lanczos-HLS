//
//  lanczos.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#include "lanczos.h"
#include <math.h>

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

uint8_t double_to_uint8(double x) {
    if (x > UINT8_MAX) {
        return UINT8_MAX;
    } else {
        return (uint8_t) x;
    }
}

double sinc(double x) {
    return sin(x)/x;
}

double lanczos_kernel(double x) {
    return x == 0 ? 1 : sinc(M_PI * x) * sinc(M_PI * x / LANCZOS_A);
}

void lanczos_interpolate_row(byte_t in[IN_WIDTH], byte_t out[OUT_WIDTH]) {
    for (int xx = 0; xx < OUT_WIDTH; xx++) {
        double x = (double) xx / SCALE;
        double sum = 0;
        for (int i = MAX(0, floor(x) - LANCZOS_A + 1); i <= MIN(IN_WIDTH - 1, floor(x) + LANCZOS_A); i++) {
            sum += in[i] * lanczos_kernel(x - i);
        }
        out[xx] = double_to_uint8(sum);
    }
}

void lanczos_interpolate_col(byte_t img[OUT_HEIGHT][OUT_WIDTH], int col) {
    // start filling from largest height first so we don't overwrite any pixels we're using
    for (int xx = OUT_HEIGHT - 1; xx >= 0; xx--) {
        double x = (double) xx / SCALE;
        double sum = 0;
        for (int i = MAX(0, floor(x) - LANCZOS_A + 1); i <= MIN(IN_HEIGHT - 1, floor(x) + LANCZOS_A); i++) {
            sum += img[i][col] * lanczos_kernel(x - i);
        }
        img[xx][col] = double_to_uint8(sum);
    }
}

void lanczos(
    byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t img_out[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
    for (int i = 0; i < IN_HEIGHT; i++) {
        for (int j = 0; j < NUM_CHANNELS; j++) {
            lanczos_interpolate_row(img_in[j][i], img_out[j][i]);
        }
    }

    for (int i = 0; i < OUT_WIDTH; i++) {
        for (int j = 0; j < NUM_CHANNELS; j++) {
            lanczos_interpolate_col(img_out[j], i);
        }
    }
}

// HLS-synthesisable variant
void lanczos_HLS(
    byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t img_out[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
    for (int i = 0; i < IN_HEIGHT; i++) {
        for (int j = 0; j < NUM_CHANNELS; j++) {
            for (int xx = 0; xx < OUT_WIDTH; xx++) {
                double x = (double) xx / SCALE;
                double sum = 0;
                for (int k = MAX(0, floor(x) - LANCZOS_A + 1); k <= MIN(IN_WIDTH - 1, floor(x) + LANCZOS_A); k++) {
                    sum += img_in[j][i][k] * (x == k ? 1 : sin(M_PI * (x - k))/(M_PI * (x - k)) * sin(M_PI * (x - k) / LANCZOS_A) / (M_PI * (x - k) / LANCZOS_A));
                }

                img_out[j][i][xx] = sum > UINT8_MAX ? UINT8_MAX : (byte_t) sum;
            }
        }
    }

    for (int i = 0; i < OUT_WIDTH; i++) {
        for (int j = 0; j < NUM_CHANNELS; j++) {
            for (int xx = OUT_HEIGHT - 1; xx >= 0; xx--) {
                double x = (double) xx / SCALE;
                double sum = 0;
                for (int k = MAX(0, floor(x) - LANCZOS_A + 1); k <= MIN(IN_HEIGHT - 1, floor(x) + LANCZOS_A); k++) {
                    sum += img_out[j][k][i] * (x == k ? 1 : sin(M_PI * (x - k))/(M_PI * (x - k)) * sin(M_PI * (x - k) / LANCZOS_A) / (M_PI * (x - k) / LANCZOS_A));
                }
                img_out[j][xx][i] = sum > UINT8_MAX ? UINT8_MAX : (byte_t) sum;
            }
        }
    }
}
