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

#include "ap_fixed.h"
#include <iostream>
using namespace std;
#include <stdio.h>
#include "params.h"

#include "hls_stream.h"


#ifndef GET_BITS_COMPUTATION
#define GET_BITS_COMPUTATION
#define GET_BITS_INPUT_1 (OUT_WIDTH + 2*LANCZOS_A*SCALE_N/SCALE_D)
#define GET_BITS_INPUT_2 (OUT_HEIGHT + 2*LANCZOS_A*SCALE_N/SCALE_D)
#define GET_BITS_INPUT_3 IN_WIDTH
#define GET_BITS_INPUT_4 ROW_WORKERS
#include "get_bits.h"
#define OUT_WIDTH_BITS 		GET_BITS_OUTPUT_1
#define OUT_HEIGHT_BITS 	GET_BITS_OUTPUT_2
#define IN_WIDTH_BITS 		GET_BITS_OUTPUT_3
#define ROW_WORKERS_BITS 	GET_BITS_OUTPUT_4
#endif

#if(OUT_HEIGHT_BITS+GET_BITS_OUTPUT_3 +ROW_WORKERS_BITS+OUT_WIDTH_BITS + IN_WIDTH)
#endif

#define NUM_BITS GET_BITS_OUTPUT

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))


/*
-0.1 < kernel_t < 1
-256*0.2 < num_t < 1.2*256
num_t is allowed to overflow, since the range can be mapped onto a 9 bit signed int without overlap.
*/


#define INTEGER_BITS 10
// for out img size of 1920,



typedef ap_uint<8> byte_el_t;
typedef ap_fixed<INTEGER_BITS+BIT_PRECISION, INTEGER_BITS> num_el_t;
typedef ap_fixed<8+BIT_PRECISION, 8> kernel_t;
typedef ap_ufixed<BIT_PRECISION, 0> fractional_t;

//typedef ap_uint<8*NUM_CHANNELS> byte_t;
//typedef ap_uint<(INTEGER_BITS+BIT_PRECISION)*NUM_CHANNELS> num_t;


//override [] operator to give back struct access

typedef ap_uint<8*NUM_CHANNELS> byte_t;
typedef ap_uint<(INTEGER_BITS+BIT_PRECISION)*NUM_CHANNELS> num_t;

void unpack_blob(num_t blob, num_el_t out[NUM_CHANNELS]);
void unpack_blob(byte_t blob, byte_el_t out[NUM_CHANNELS]);
num_t pack_blob(num_el_t in[NUM_CHANNELS]);
byte_t pack_blob(byte_el_t in[NUM_CHANNELS]);

typedef ap_uint<OUT_WIDTH_BITS> row_major_counter_t;
typedef ap_uint<OUT_HEIGHT_BITS> col_major_counter_t;
typedef ap_uint<ROW_WORKERS> row_minor_counter_t;
typedef ap_uint<IN_WIDTH_BITS> col_minor_counter_t;

typedef struct{
	ap_uint<8> channel[NUM_CHANNELS];
} rgb_pixel_t;

// GCD is defined in stb.cpp 
int gcd(int, int);

static const int SCALE_GCD = gcd(OUT_WIDTH, IN_WIDTH);

#define SCALE ((double)SCALE_N/SCALE_D)
#define SCALE_INT (OUT_WIDTH/IN_WIDTH)
#define SCALE_IS_INT (OUT_WIDTH % IN_WIDTH == 0)

//void lanczos(
//	byte_t in_img[IN_HEIGHT][IN_WIDTH],
//	byte_t out_img[OUT_HEIGHT][OUT_WIDTH]
//);

typedef hls::stream<byte_t> & stream_t;

extern void lanczos(
	stream_t streamin,
	stream_t streamout
);

#endif /* lanczos_h */
