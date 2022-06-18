//
//  main.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#include <stdio.h>
#include "lanczos.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define DEBUG 1

byte_t red_in[IN_HEIGHT][IN_WIDTH];
byte_t blue_in[IN_HEIGHT][IN_WIDTH];
byte_t green_in[IN_HEIGHT][IN_WIDTH];

byte_t red_out[OUT_HEIGHT][OUT_WIDTH];
byte_t blue_out[OUT_HEIGHT][OUT_WIDTH];
byte_t green_out[OUT_HEIGHT][OUT_WIDTH];

int main(int argc, char* argv[]) {
    int width;
    int height;
    int channels;

#ifndef DEBUG
    if (argc < 2) {
        fputs("Image path not provided", stderr);
        return EXIT_FAILURE;
    }
    byte_t* img = stbi_load(argv[1], &width, &height, &channels, 3);
#else
    byte_t* img = stbi_load("/Users/pkbeam/Documents/Programming/LanczosUpscaler/LanczosUpscaler/img/IMG_0212.jpeg", &width, &height, &channels, 3);
#endif

    if (img == NULL) {
        printf("Image was not loaded successfully.\n");
        return EXIT_FAILURE;
    }

    if (width != IN_WIDTH || height != IN_HEIGHT) {
        printf("Image has wrong dimensions (%i x %i).\n", width, height);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < width * height * channels; i += channels) {
        int row = i/IN_WIDTH;
        //printf("RGB (%i %i %i)\n", img[i], img[i + 1], img[i+2]);
        red_in[row][i] = img[i];
        blue_in[row][i] = img[i + 1];
        green_in[row][i] = img[i + 2];
    }

    lanczos(red_in, blue_in, green_in, red_out, blue_out, green_out);

    return 0;
}
