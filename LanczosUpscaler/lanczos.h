//
//  lanczos.h
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#ifndef lanczos_h
#define lanczos_h

#include <stdio.h>

#define IN_WIDTH  1920
#define IN_HEIGHT 1080

#define OUT_WIDTH  3840
#define OUT_HEIGHT 2160

#define SCALE (OUT_WIDTH/IN_WIDTH)

#define LANCZOS_A 2

typedef uint8_t byte_t;

// HLS target function
void lanczos_HLS(
    byte_t red_in[IN_HEIGHT][IN_WIDTH],
    byte_t blue_in[IN_HEIGHT][IN_WIDTH],
    byte_t green_in[IN_HEIGHT][IN_WIDTH],
    byte_t red_out[OUT_HEIGHT][OUT_WIDTH],
    byte_t blue_out[OUT_HEIGHT][OUT_WIDTH],
    byte_t green_out[OUT_HEIGHT][OUT_WIDTH]
);

void lanczos(
    byte_t red_in[IN_HEIGHT][IN_WIDTH],
    byte_t blue_in[IN_HEIGHT][IN_WIDTH],
    byte_t green_in[IN_HEIGHT][IN_WIDTH],
    byte_t red_out[OUT_HEIGHT][OUT_WIDTH],
    byte_t blue_out[OUT_HEIGHT][OUT_WIDTH],
    byte_t green_out[OUT_HEIGHT][OUT_WIDTH]
);

#endif /* lanczos_h */
