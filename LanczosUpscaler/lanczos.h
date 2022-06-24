//
//  lanczos.h
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//


/* Copy the following code into a new header file "params.h" in the source directory
#ifndef PARAMS_H
#define PARAMS_H

#define OUT_WIDTH  (IN_WIDTH*3)
#define OUT_HEIGHT (IN_HEIGHT*3)

#define OUT_DIR "/home/ezra/COMP4601/Project/LanczosUpscaler/img/"
#define IN_IMG "2022-06-24_01-17.png"

#define IN_WIDTH  162
#define IN_HEIGHT 89

#define OUT_IMG "expected.png"
#define OUT_IMG_OBSERVED "observed.png"

#define NUM_CHANNELS 3
#define LANCZOS_A 2

#define BIT_PRECISION 8

#endif
*/

#ifndef lanczos_h
#define lanczos_h

#include <iostream>
using namespace std;
#include <stdio.h>
#include "ap_fixed.h"
#include "params.h"

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)


typedef ap_uint<8> byte_t;

/*
-0.1 < kernel_t < 1
-256*0.2 < num_t < 1.2*256
num_t is allowed to overflow, since the range can be mapped onto a 9 bit signed int without overlap.
*/


typedef ap_fixed<BIT_PRECISION+2,2> kernel_t;
typedef ap_fixed<BIT_PRECISION+9,9> num_t;

// Awful hack to get GCD calculated at compile time. Guess we are programming in C.
// https://stackoverflow.com/a/78794

int gcd(int, int);

static const int SCALE_GCD = gcd(OUT_WIDTH, IN_WIDTH);

#define SCALE ((double)OUT_WIDTH/IN_WIDTH)
#define SCALE_N (OUT_WIDTH/SCALE_GCD)
#define SCALE_D (IN_WIDTH/SCALE_GCD)
#define SCALE_INT (OUT_WIDTH/IN_WIDTH)
#define SCALE_IS_INT (OUT_WIDTH % IN_WIDTH == 0)

void lanczos(
    byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t img_out[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
);

#endif /* lanczos_h */
