//
//  main.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#include <stdio.h>
#include "lanczos.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#define DEBUG 1

typedef struct {
    byte_t channel[NUM_CHANNELS];
} rgb_pixel_t;

byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH];
byte_t img_out[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH];

rgb_pixel_t img_interlaced_out[OUT_HEIGHT * OUT_WIDTH];

#define OUT_DIR "/Users/pkbeam/Documents/Programming/LanczosUpscaler/LanczosUpscaler/img/"
#define IN_IMG "IMG_0212.jpeg"
#define OUT_IMG "out.png"

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
    lanczos_HLS(img_in, img_out);

    // copy image data back
    for (int i = 0; i < OUT_WIDTH * OUT_HEIGHT; i++) {
        int row = i / OUT_WIDTH;
        int col = i % OUT_WIDTH;

        rgb_pixel_t pixel = {};
        for (int j = 0; j < NUM_CHANNELS; j++) {
            pixel.channel[j] = img_out[j][row][col];
        }
        img_interlaced_out[i] = pixel;
    }

    // save data to image
    stbi_write_png(OUT_DIR OUT_IMG, OUT_WIDTH, OUT_HEIGHT, NUM_CHANNELS, img_interlaced_out, OUT_WIDTH * NUM_CHANNELS);

    return 0;
}
