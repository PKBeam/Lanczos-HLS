//
//  lanczos.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#include "lanczos.h"
#include "hls_math.h"

typedef ap_fixed<BIT_PRECISION+3,3> kernel_in_t;

#define FORALL(a,n) for (a = 0; a < n; a++)
#define PI ((kernel_in_t)M_PI)

kernel_t lanczos_kernel(kernel_in_t x){
	if (x==0){
		return (kernel_t) 1.0;
	} else {
		return (kernel_t) (LANCZOS_A*hls::sinpi(x)*hls::sinpi(x / LANCZOS_A) / (PI*PI*x*x));
	}
}

byte_t clamp_to_byte(num_t x){
	if (x[BIT_PRECISION+8]){
		return x[BIT_PRECISION+7] ? 0 : 255;
	} else {
		return x;
	}

}
// HLS-synthesisable variant
void lanczos(
    byte_t img_input[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t img_output[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
	num_t img_processed_rows[NUM_CHANNELS][IN_HEIGHT][OUT_WIDTH];
    for (int j = 0; j < NUM_CHANNELS; j++) {
    	for (int i = 0; i < IN_HEIGHT; i++) {
			for (int xx = 0; xx < OUT_WIDTH; xx++) {
				int floorx = (SCALE_D * xx) / SCALE_N;
				num_t sum = 0;
				for (int k = 1 - LANCZOS_A; k <= LANCZOS_A; k++){
					int kk = k + floorx;
					if (kk >= 0 && kk < IN_WIDTH){
						sum += img_input[j][i][kk] * lanczos_kernel((kernel_in_t)((num_t)(xx*SCALE_D-kk*SCALE_N)/SCALE_N));
					}
                }
				img_processed_rows[j][i][xx] = sum;
			}
		}
		for (int i = 0; i < OUT_WIDTH; i++) {
            for (int xx = 0; xx < OUT_HEIGHT; xx++) {
				int floorx = (SCALE_D * xx) / SCALE_N;
            	num_t sum = 0;
				for (int k = 1 - LANCZOS_A; k <= LANCZOS_A; k++){
					int kk = k + floorx;
					if (kk >= 0 && kk < IN_HEIGHT){
						sum += img_processed_rows[j][kk][i] * lanczos_kernel((kernel_in_t)((num_t)(xx*SCALE_D-kk*SCALE_N)/SCALE_N));
					}
                }
				img_output[j][xx][i] = clamp_to_byte(sum);
            }
        }
    }
    return;
}
