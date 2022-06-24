//
//  main.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//
#include "lanczos.h"
#include <math.h>

#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"

#define DEBUG 1

typedef struct {
    byte_t channel[NUM_CHANNELS];
} rgb_pixel_t;

byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH];
byte_t img_out_ex[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH];
byte_t img_out_ob[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH];

rgb_pixel_t img_interlaced_out_ex[OUT_HEIGHT * OUT_WIDTH];

rgb_pixel_t img_interlaced_out_ob[OUT_HEIGHT * OUT_WIDTH];



byte_t double_to_uint8(double x) {
    if (x > UINT8_MAX) {
        return UINT8_MAX;
    } else if (x < 0){
        return 0;
    } else {
    	return (byte_t) x;
    }
}

double sinc(double x) {
    return sin(x)/x;
}

double lanczos_kernel(double x) {
    return sinc(M_PI * x) * sinc(M_PI * x / LANCZOS_A);
}

void lanczos_interpolate_row(byte_t in[IN_WIDTH], byte_t out[OUT_WIDTH]) {
    for (int xx = 0; xx < OUT_WIDTH; xx++) {
        if (SCALE_IS_INT && xx % SCALE_INT == 0) {
            out[xx] = in[xx / SCALE_INT];
            continue;
        }
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
        if (SCALE_IS_INT && xx % SCALE_INT == 0) {
            img[xx][col] = img[xx / SCALE_INT][col];
            continue;
        }
        double x = (double) xx / SCALE;
        double sum = 0;
        for (int i = MAX(0, floor(x) - LANCZOS_A + 1); i <= MIN(IN_HEIGHT - 1, floor(x) + LANCZOS_A); i++) {
            sum += img[i][col] * lanczos_kernel(x - i);
        }
        img[xx][col] = double_to_uint8(sum);
    }
}

void lanczos_expected(
    byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t img_out[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
    for (int i = 0; i < IN_HEIGHT; i++) {
        for (int j = 0; j < NUM_CHANNELS; j++) {
            lanczos_interpolate_row(img_in[j][i], img_out[j][i]);
        }
    }

    for (int i1 = 0; i1 < OUT_WIDTH; i1++) {
        for (int j = 0; j < NUM_CHANNELS; j++) {
            lanczos_interpolate_col(img_out[j], i1);
        }
    }

    return;
}


int main(int argc, char* argv[]) {
    int width;
    int height;
    int channels;

    rgb_pixel_t* img = (rgb_pixel_t*) stbi_load(OUT_DIR IN_IMG, &width, &height, &channels, NUM_CHANNELS);

    // error checking
    if (img == NULL) {
        printf("Image was not loaded successfully.\n");
        return EXIT_FAILURE;
    }

    if (width != IN_WIDTH || height != IN_HEIGHT) {
        printf("Image has wrong dimensions (%i x %i).\n", width, height);
        return EXIT_FAILURE;
    }

    if (channels != NUM_CHANNELS) {
        printf("Image has wrong amount of channels (expected %i, got %i).\n", NUM_CHANNELS, channels);
        return EXIT_FAILURE;
    }

    // read image data
    for (int i = 0; i < IN_WIDTH * IN_HEIGHT; i++) {
        int row = i / IN_WIDTH;
        int col = i % IN_WIDTH;

        for (int j = 0; j < NUM_CHANNELS; j++) {
            img_in[j][row][col] = img[i].channel[j];
        }
    }

    // apply lanczos
    lanczos(img_in, img_out_ob);
    printf("hello\n");
    lanczos_expected(img_in, img_out_ex);

    printf("hi\n");
    double err = 0;
    // copy image data back
    for (int i = 0; i < OUT_WIDTH * OUT_HEIGHT; i++) {
        int row = i / OUT_WIDTH;
        int col = i % OUT_WIDTH;

        rgb_pixel_t pixel_ex = {};
        rgb_pixel_t pixel_ob = {};
        for (int j = 0; j < NUM_CHANNELS; j++) {
        	byte_t expected_val  = img_out_ex[j][row][col];
        	byte_t observed_val  = img_out_ob[j][row][col];

        	pixel_ex.channel[j] = expected_val;
            pixel_ob.channel[j] = observed_val;
            int diff = (int)expected_val - (int)observed_val;
            err += diff*diff;
        }
        img_interlaced_out_ex[i] = pixel_ex;
        img_interlaced_out_ob[i] = pixel_ob;
    }
    printf("RMS err: %.3f\n", sqrt((double)err/(NUM_CHANNELS*OUT_WIDTH*OUT_HEIGHT)));
    // save data to both observed and expected
    stbi_write_png(OUT_DIR OUT_IMG_EX, OUT_WIDTH, OUT_HEIGHT, NUM_CHANNELS, img_interlaced_out_ex, OUT_WIDTH * NUM_CHANNELS);
    stbi_write_png(OUT_DIR OUT_IMG_OB, OUT_WIDTH, OUT_HEIGHT, NUM_CHANNELS, img_interlaced_out_ob, OUT_WIDTH * NUM_CHANNELS);

    return 0;
}
