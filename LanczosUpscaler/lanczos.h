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

typedef uint8_t byte_t;

// HLS target function
void lanczos(
    byte_t red_in[IN_HEIGHT][IN_WIDTH],
    byte_t blue_in[IN_HEIGHT][IN_WIDTH],
    byte_t grn_in[IN_HEIGHT][IN_WIDTH],
    byte_t red_out[OUT_HEIGHT][OUT_WIDTH],
    byte_t blue_out[OUT_HEIGHT][OUT_WIDTH],
    byte_t grn_out[OUT_HEIGHT][OUT_WIDTH]
);

#endif /* lanczos_h */
