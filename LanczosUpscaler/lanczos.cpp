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

ColWorkers c(0);
RowWorkers r(0);

void fillColBuffer(byte_t in_img[IN_HEIGHT][IN_WIDTH], num_t (* buf)[ROW_WORKERS]){
	kernel_t kernel_vals[2*LANCZOS_A];
	c.seek_write_index(0);
	fillColBuffer_rowWorkerWidth:
	for (int i = 0; i < ROW_WORKERS; i++){
		fillColBuffer_rowWorkerWidth_kernelVals:
		for(int j = 0; j < 2*LANCZOS_A; j++){
			#pragma HLS unroll
			kernel_vals[j] = lanczos_kernel(c.in_idx - 2*LANCZOS_A+j, c.get_out_pos(), (scale_t)SCALE);
		}
		c.exec(in_img, kernel_vals, buf);
	}
}

void fillRowBuffer(num_t (* buf)[ROW_WORKERS], byte_t (* out_img)[OUT_WIDTH]){
	kernel_t kernel_vals[2*LANCZOS_A];
	// row takes new input for every new buffer given to it.
	r.initialize(buf);
	fillRowBuffer_outWidth:
	for (int i = 0; i < OUT_WIDTH; i++){
		#pragma HLS PIPELINE
		fillRowBuffer_outWidth_kernelVals:
		for(int j = 0; j < 2*LANCZOS_A; j++){
			#pragma HLS unroll
			kernel_vals[j] = lanczos_kernel(r.in_idx - 2*LANCZOS_A+j, r.get_out_pos(), (scale_t)SCALE);
		}
		r.exec(buf, kernel_vals, out_img);
	}
}

void stream_out(byte_t buf2[ROW_WORKERS][OUT_WIDTH], byte_t (* out_channel)[OUT_WIDTH]){
	for(int i = 0; i < ROW_WORKERS; i++){

		for(int j = 0; j < OUT_WIDTH; j++){
			#pragma HLS PIPELINE
			#pragma HLS UNROLL factor=4
			#pragma HLS LOOP_FLATTEN off
			out_channel[i][j] = buf2[i][j];
		}
	}

}
void process_channel(byte_t (* in_channel)[IN_WIDTH], byte_t (* out_channel)[OUT_WIDTH]){
	// Column worker takes new input for every new channel.
	c.initialize(in_channel);
	lanczosComputeBuffers:
	for(int i = 0; i < (OUT_HEIGHT+ROW_WORKERS-1)/ROW_WORKERS; i++){
		#pragma HLS DATAFLOW
		num_t buf1[IN_WIDTH][ROW_WORKERS];
		#pragma HLS ARRAY_PARTITION variable=buf1 complete dim=2
		byte_t buf2[ROW_WORKERS][OUT_WIDTH];
		#pragma HLS ARRAY_PARTITION variable=buf2 complete dim=1
		byte_t (* out_img_ptr)[OUT_WIDTH] = out_channel + ROW_WORKERS*i;
		fillColBuffer(in_channel, buf1); // byte to num_t
		fillRowBuffer(buf1, buf2); // num_T to byte
		stream_out(buf2, out_img_ptr);
	}
}


void lanczos(
    byte_t in_img[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte_t out_img[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
#pragma HLS INTERFACE axis register both port=in_img
#pragma HLS INTERFACE axis register both port=out_img
	// Perform column lengthening first, then row lengthening

	colourChannels:
	for (int chan=0; chan< NUM_CHANNELS; chan++){
		process_channel(in_img[chan], out_img[chan]);
	}
}
