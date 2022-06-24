//
//  lanczos.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#include "lanczos.h"
#include "hls_math.h"

typedef ap_fixed<BIT_PRECISION+3,3> kernel_in_t;

#define PI ((kernel_in_t)M_PI)
kernel_t lanczos_kernel(kernel_in_t x){
	kernel_t out = x==0? (kernel_t) 1.0 : (kernel_t) (LANCZOS_A*hls::sinpi(x)*hls::sinpi(x / LANCZOS_A) / (PI*PI*x*x));
	return out;
}
#undef PI

byte_t clamp_to_byte(num_t x){
	if (x[BIT_PRECISION+8]){
		return x[BIT_PRECISION+7] ? 0 : 255;
	} else {
		return x;
	}

}

// HLS-synthesisable variant
void lanczos(
    byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t img_out[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
	num_t img_processed_rows[NUM_CHANNELS][IN_HEIGHT][OUT_WIDTH];
    for (int j = 0; j < NUM_CHANNELS; j++) {
    	for (int i = 0; i < IN_HEIGHT; i++) {
			for (int xx = 0; xx < OUT_WIDTH; xx++) {
				int floorx = (SCALE_D * xx) / SCALE_N;
				int start_idx = MAX(0, floorx - LANCZOS_A + 1);
				int end_idx = MIN(IN_WIDTH - 1, floorx + LANCZOS_A);
				num_t sum = 0;
				for (int k = start_idx; k <= end_idx; k++) {
					sum += img_in[j][i][k] * lanczos_kernel((kernel_in_t)((num_t)(xx*SCALE_D-k*SCALE_N)/SCALE_N));
				}
				img_processed_rows[j][i][xx] = sum;
			}
		}
		for (int i = 0; i < OUT_WIDTH; i++) {
            for (int xx = OUT_HEIGHT - 1; xx >= 0; xx--) {
				int floorx = (SCALE_D * xx) / SCALE_N;
				int start_idx = MAX(0, floorx - LANCZOS_A + 1);
				int end_idx = MIN(IN_HEIGHT - 1, floorx + LANCZOS_A);
            	num_t sum = 0;
				for (int k = start_idx; k <= end_idx; k++)
				{
                    sum += img_processed_rows[j][k][i] * lanczos_kernel((kernel_in_t)((num_t)(xx*SCALE_D-k*SCALE_N)/SCALE_N));
                }

                img_out[j][xx][i] = clamp_to_byte(sum);
            }
        }
    }
    return;
}
