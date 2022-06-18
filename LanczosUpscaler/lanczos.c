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



double sinc(double x) {
    return sin(x)/x;
}

double lanczos_kernel(double x) {
    return sinc(M_PI * x) * sinc(M_PI * x / LANCZOS_A);
}

void lanczos_interpolate_row(byte_t in[IN_WIDTH], byte_t out[OUT_WIDTH]) {
    for (int xx = 0; xx < OUT_WIDTH; xx++) {
        if (xx % SCALE == 0) {
            out[xx] = in[xx / SCALE];
            continue;
        }
        double x = (double) xx / SCALE;
        double sum = 0;
        for (int i = MAX(0, floor(x) - LANCZOS_A + 1); i <= MIN(IN_WIDTH - 1, floor(x) + LANCZOS_A); i++) {
            sum += in[i] * lanczos_kernel(x - i);
        }
        out[xx] = (int)sum;
    }
}

void lanczos_interpolate_col(byte_t img[OUT_HEIGHT][OUT_WIDTH], int col) {
    // start filling from largest height first so we don't overwrite any pixels we're using
    for (int xx = OUT_HEIGHT - 1; xx >= 0; xx--) {
        if (xx % SCALE == 0) {
            img[xx][col] = img[xx / SCALE][col];
            continue;
        }
        double x = (double) xx / SCALE;
        double sum = 0;
        for (int i = MAX(0, floor(x) - LANCZOS_A + 1); i <= MIN(IN_HEIGHT - 1, floor(x) + LANCZOS_A); i++) {
            sum += img[i][col] * lanczos_kernel(x - i);
        }
        img[xx][col] = (int)sum;
    }
}

void lanczos(
    byte_t red_in[IN_HEIGHT][IN_WIDTH],
    byte_t blue_in[IN_HEIGHT][IN_WIDTH],
    byte_t green_in[IN_HEIGHT][IN_WIDTH],
    byte_t red_out[OUT_HEIGHT][OUT_WIDTH],
    byte_t blue_out[OUT_HEIGHT][OUT_WIDTH],
    byte_t green_out[OUT_HEIGHT][OUT_WIDTH]
) {
    for (int i = 0; i < IN_HEIGHT; i++) {
        lanczos_interpolate_row(red_in[i], red_out[i]);
        lanczos_interpolate_row(blue_in[i], blue_out[i]);
        lanczos_interpolate_row(green_in[i], green_out[i]);
    }

    for (int i = 0; i < OUT_WIDTH; i++) {
        lanczos_interpolate_col(red_out, i);
        lanczos_interpolate_col(blue_out, i);
        lanczos_interpolate_col(green_out, i);
    }

    return;
}
