//
//  lanczos.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//

#include "math.h"
#include "lanczos.h"
#include "worker.h"
#include "kernel.h"
//#include "hls_math.h"

num_t buf1[OUT_HEIGHT][ROW_WORKERS], buf2[OUT_HEIGHT][ROW_WORKERS];

num_t (* buf_read)[ROW_WORKERS] = buf1;
num_t (* buf_write)[ROW_WORKERS] = buf2;


// old lanczos
//void lanczos(
//    byte_t img_input[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
//    byte_t img_output[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
//) {
////	kernel_t kernel_vals[4] = {0.25, 0.25, 0.25, 0.25};
//	RowWorker r_worker(0);
//	ColWorker c_worker(0);
//	for (int chan=0; chan< NUM_CHANNELS; chan++){
//		// refreshes buffers and resets counters
//		r_worker.initialize(img_input[chan]);
//		for(int i = 0; i < OUT_WIDTH; i++){
//			// get kernel values
//			kernel_t kernel_vals[2*LANCZOS_A];
//
//			for(int j=0; j < 2*LANCZOS_A; j++){
//				kernel_vals[j] = lanczos_kernel(r_worker.in_idx -2*LANCZOS_A+j, r_worker.out_idx - r_worker.offset, (scale_t)SCALE);
//			}
//
//			r_worker.exec(img_input[chan], kernel_vals, img_processed_rows[chan]);
//		}
//
//		// refreshes buffers and resets counters
//		c_worker.initialize(img_processed_rows[chan]);
//		for(int i = 0; i < OUT_HEIGHT; i++){
//			// get kernel values
//			kernel_t kernel_vals[2*LANCZOS_A];
//
//			for(int j=0; j < 2*LANCZOS_A; j++){
//
//				kernel_vals[j] = lanczos_kernel(c_worker.in_idx - 2*LANCZOS_A+j, c_worker.out_idx - c_worker.offset, (scale_t)SCALE);
//			}
//
//			c_worker.exec(img_processed_rows[chan], kernel_vals, img_output[chan]);
//		}
//	}
//}


void fillColBuffer(byte_t in_img[IN_HEIGHT][IN_WIDTH], num_t (* buf)[ROW_WORKERS], ColWorkers& proc){
	// TODO: kern = get kernel value
	static kernel_t kernel_vals[2*LANCZOS_A];
	proc.seek_write_index(0);
	for (int i = 0; i < ROW_WORKERS; i++){
		for(int j=0; j < 2*LANCZOS_A; j++){
			kernel_vals[j] = lanczos_kernel(proc.in_idx - 2*LANCZOS_A+j, proc.get_out_pos(), (scale_t)SCALE);
		}
		proc.exec(in_img, kernel_vals, buf);
	}
}

void fillRowBuffer(num_t (* buf)[ROW_WORKERS], byte_t (* out_img)[OUT_WIDTH], RowWorkers& proc){
	static kernel_t kernel_vals[2*LANCZOS_A];
	// row takes new input for every new buffer given to it.
	proc.initialize(buf);
	for (int i = 0; i < OUT_WIDTH; i++){
		for(int j=0; j < 2*LANCZOS_A; j++){
			kernel_vals[j] = lanczos_kernel(proc.in_idx - 2*LANCZOS_A+j, proc.get_out_pos(), (scale_t)SCALE);
		}
		proc.exec(buf, kernel_vals, out_img);
	}
}

void lanczos(
    byte_t in_img[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t out_img[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {

	// Perform column lengthening first, then row lengthening
	ColWorkers c(0);
	RowWorkers r(0);
	num_t (* temp)[ROW_WORKERS];

	for (int chan=0; chan< NUM_CHANNELS; chan++){
		byte_t (* out_img_ptr)[OUT_WIDTH] = out_img[chan];
		// Column worker takes new input for every new channel.
		c.initialize(in_img[chan]);

		for(int i = 0; i < ceil((double)OUT_HEIGHT/ROW_WORKERS); i++){
			fillColBuffer(in_img[chan], buf_write, c); // byte to num_t
			temp = buf_write;
			buf_write = buf_read;
			buf_read = temp;

			// col workers work on buf_read -> row worker work on input -> BUF_WRTE
			fillRowBuffer(buf_read, out_img_ptr, r); // num_T to byte
			// Shift location of image write down
			out_img_ptr += ROW_WORKERS;
		}
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
