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
#define NUM_CHANNELS 3

typedef struct {
    byte_t r;
    byte_t g;
    byte_t b;
} rgb_pixel_t;

byte_t red_in[IN_HEIGHT][IN_WIDTH];
byte_t blue_in[IN_HEIGHT][IN_WIDTH];
byte_t green_in[IN_HEIGHT][IN_WIDTH];

byte_t red_out[OUT_HEIGHT][OUT_WIDTH];
byte_t blue_out[OUT_HEIGHT][OUT_WIDTH];
byte_t green_out[OUT_HEIGHT][OUT_WIDTH];

rgb_pixel_t img_out[OUT_HEIGHT * OUT_WIDTH];

#define OUT_DIR "/Users/pkbeam/Documents/Programming/LanczosUpscaler/LanczosUpscaler/img/"

int main(int argc, char* argv[]) {
    int width;
    int height;
    int channels;

    rgb_pixel_t* img = (rgb_pixel_t*) stbi_load(OUT_DIR "IMG_0212.jpeg", &width, &height, &channels, NUM_CHANNELS);

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

        red_in[row][col] = img[i].r;
        green_in[row][col] = img[i].g;
        blue_in[row][col] = img[i].b;
    }

    // apply lanczos
    lanczos(red_in, blue_in, green_in, red_out, blue_out, green_out);

    // copy image data back
    for (int i = 0; i < OUT_WIDTH * OUT_HEIGHT; i++) {
        int row = i / OUT_WIDTH;
        int col = i % OUT_WIDTH;

        img_out[i] = (rgb_pixel_t) {
            red_out[row][col],
            green_out[row][col],
            blue_out[row][col],
        };
    }

    stbi_write_png(OUT_DIR "out.png", OUT_WIDTH, OUT_HEIGHT, NUM_CHANNELS, img_out, OUT_WIDTH * NUM_CHANNELS);

    return 0;
}
