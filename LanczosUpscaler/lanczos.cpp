//
//  lanczos.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#include "lanczos.h"
#include "hls_math.h"

num_t img_processed_rows[NUM_CHANNELS][IN_HEIGHT][OUT_WIDTH];

void lanczos(
    byte_t img_input[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t img_output[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
	// unroll for each colour channel
	for (int j = 0; j < NUM_CHANNELS; j++) {
	#pragma HLS UNROLL
		// TODO: init workers
	}
}

/* old implementation
 *
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
*/
