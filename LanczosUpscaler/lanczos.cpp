//
//  lanczos.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#include "lanczos.h"
#include "hls_math.h"

typedef float kernel_in_t;

#define FORALL(a,n) for (a = 0; a < n; a++)
#define GET(siz, arr, idx) (idx < 0 || idx >= siz) ? img_input[j][i][idx];
#define PI ((kernel_in_t)M_PI)

kernel_t lanczos_kernel(kernel_in_t x){
	if (x==0){
		return (kernel_t) 1.0;
	}
	return (kernel_t) (LANCZOS_A*hls::sinpi(x)*hls::sinpi(x / LANCZOS_A) / (PI*PI*x*x));
//	return (kernel_t) ((1-x*x)*(1-x*x/(LANCZOS_A*LANCZOS_A))*(1-x*x/(LANCZOS_A*LANCZOS_A)));
}

byte_t clamp_to_byte(num_t x){
//	if (x[BIT_PRECISION+8]){
//		return x[BIT_PRECISION+7] ? 0 : 255;
//	} else {
//		return x;
//	}
	if (x  > 255){
		x = 255;
	} else if (x < 0){
		x = 0;
	}
	return (byte_t)x;
}
template <class T, int siz>
T shift_left(T arr[siz], T next){
	T out = arr[siz-1];
	for(int i = siz-1; i > 0; i--){
		arr[i] = arr[i-1];
	}
	arr[0] = next;
	return out;
}
template <class T, int siz>
T shift_right(T arr[siz], T next){
	T out = arr[0];
	for(int i = 0; i < siz-1; i++){
		arr[i] = arr[i+1];
	}
	arr[siz-1] = next;
	return out;
}

// HLS-synthesisable variant
void lanczos(
    byte_t img_input[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t img_output[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
	#pragma HLS INTERFACE m_axi depth=10 port=img_input
	num_t img_processed_rows[NUM_CHANNELS][IN_HEIGHT][OUT_WIDTH];
	byte_t img_input_2[IN_WIDTH];
    for (int j = 0; j < NUM_CHANNELS; j++) {
    	for (int i = 0; i < IN_HEIGHT; i++) {
    		// Setup index for each row.
    		int in_idx = 0;

    		// initialize buffer with first few values of input
    		byte_t img_input_buffer[2*LANCZOS_A];

    		for (int l = 0; l < LANCZOS_A*2; l++){
				#pragma HLS UNROLL complete
    			img_input_buffer[l] = l < LANCZOS_A-1 ? (byte_t)0 : img_input[j][i][in_idx++];
    		}
			for (int xx = 0; xx < OUT_WIDTH; xx++) {
				// Check whether x is large enough to take next input var
				if ((in_idx-LANCZOS_A)*SCALE_N < xx*SCALE_D){
					shift_right<byte_t, 2*LANCZOS_A>(img_input_buffer, in_idx >= IN_WIDTH ? (byte_t)0 : img_input[j][i][in_idx++]);
				}
				num_t sum = 0;
				byte_t *ptr = img_input_buffer;
				for (int k = -2*LANCZOS_A; k < 0; k++){
					int kk = in_idx + k;
					sum += *(ptr++) * lanczos_kernel((kernel_in_t)((num_t)(xx*SCALE_D-kk*SCALE_N)/(num_t)SCALE_N));
                }
				img_processed_rows[j][i][xx] = sum;
			}
		}
		for (int i = 0; i < OUT_WIDTH; i++) {
			// Setup index for each row.
			int in_idx = 0;
			// initialize buffer with first few values of input
			num_t img_input_buffer[2*LANCZOS_A];
			for (int l = 0; l < LANCZOS_A*2; l++){
				#pragma HLS UNROLL complete
				img_input_buffer[l] = l < LANCZOS_A-1 ? (num_t)0 : img_processed_rows[j][in_idx++][i];
			}
			for (int xx = 0; xx < OUT_HEIGHT; xx++) {
				// Check whether x is large enough to take next input var
				if ((in_idx-LANCZOS_A)*SCALE_N < xx*SCALE_D){
					shift_right<num_t, 2*LANCZOS_A>(img_input_buffer, in_idx >= IN_HEIGHT ? (num_t)0 : img_processed_rows[j][in_idx++][i]);
				}
				num_t sum = 0;
				num_t *ptr = img_input_buffer;
				for (int k = -2*LANCZOS_A; k < 0; k++){
					int kk = in_idx + k;
					sum += *(ptr++) * lanczos_kernel((kernel_in_t)((num_t)(xx*SCALE_D-kk*SCALE_N)/(num_t)SCALE_N));
				}
				img_output[j][xx][i] = clamp_to_byte(sum);
            }
		}
    }
    return;
}
