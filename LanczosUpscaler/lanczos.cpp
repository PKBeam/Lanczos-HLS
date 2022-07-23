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

num_t buf[2][IN_WIDTH][ROW_WORKERS];

void fillColBuffer(byte_t in_img[IN_HEIGHT][IN_WIDTH], num_t (* buf)[ROW_WORKERS], ColWorkers& proc){
	kernel_t kernel_vals[2*LANCZOS_A];
	proc.seek_write_index(0);
	fillColBuffer_rowWorkerWidth:
	for (int i = 0; i < ROW_WORKERS; i++){

		fillColBuffer_rowWorkerWidth_kernelVals:
		for(int j = 0; j < 2*LANCZOS_A; j++){
			#pragma HLS unroll
			kernel_vals[j] = lanczos_kernel(proc.in_idx - 2*LANCZOS_A+j, proc.get_out_pos(), (scale_t)SCALE);
		}
		proc.exec(in_img, kernel_vals, buf);
	}
}

void fillRowBuffer(num_t (* buf)[ROW_WORKERS], byte_t (* out_img)[OUT_WIDTH], RowWorkers& proc){
	kernel_t kernel_vals[2*LANCZOS_A];
	// row takes new input for every new buffer given to it.
	proc.initialize(buf);
	fillRowBuffer_outWidth:
	for (int i = 0; i < OUT_WIDTH; i++){
		fillRowBuffer_outWidth_kernelVals:
		for(int j = 0; j < 2*LANCZOS_A; j++){
			#pragma HLS unroll
			kernel_vals[j] = lanczos_kernel(proc.in_idx - 2*LANCZOS_A+j, proc.get_out_pos(), (scale_t)SCALE);
		}
		proc.exec(buf, kernel_vals, out_img);
	}
}

void lanczos(
    byte_t in_img[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t out_img[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
#pragma HLS INTERFACE axis register both port=in_img
#pragma HLS ARRAY_PARTITION variable=buf complete dim=1

	// Perform column lengthening first, then row lengthening
	ColWorkers c(0);
	//#pragma HLS ARRAY_PARTITION variable=c.inputBuffers complete dim=1
	//#pragma HLS ARRAY_PARTITION variable=c.inputBuffers complete dim=2
	RowWorkers r(0);
	//#pragma HLS ARRAY_PARTITION variable=r.inputBuffers complete dim=1
	//#pragma HLS ARRAY_PARTITION variable=r.inputBuffers complete dim=2
	colourChannels:
	for (int chan=0; chan< NUM_CHANNELS; chan++){
		byte_t (* out_img_ptr)[OUT_WIDTH] = out_img[chan];
		// Column worker takes new input for every new channel.
		c.initialize(in_img[chan]);
		bool is_write_buf = 0;
		lanczosComputeBuffers:
		for(int i = 0; i < ceil((double)OUT_HEIGHT/ROW_WORKERS); i++){
			fillColBuffer(in_img[chan], buf[is_write_buf], c); // byte to num_t
			// col workers work on buf_read -> row worker work on input -> BUF_WRTE
			is_write_buf = !is_write_buf;
			fillRowBuffer(buf[!is_write_buf], out_img_ptr, r); // num_T to byte
			// Shift location of image write down
			out_img_ptr += ROW_WORKERS;

		}
	}
}
