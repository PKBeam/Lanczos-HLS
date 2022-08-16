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

// STREAM TODO
#include "hls_stream.h"

ColWorkers c(0);
RowWorkers r(0);


void fillColBuffer(stream_t in_img , num_t buf[IN_WIDTH][ROW_WORKERS]){
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



void fillRowBuffer(num_t buf[IN_WIDTH][ROW_WORKERS], byte_t out_img[ROW_WORKERS][OUT_WIDTH]){
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

void stream_out(byte_t buf2[ROW_WORKERS][OUT_WIDTH], stream_t out_channel){
	static col_major_counter_t pos = 0;
	int lim = MIN(OUT_HEIGHT - (int)pos, ROW_WORKERS);
	for(row_minor_counter_t i = 0; i < lim; i++){
		for (row_major_counter_t j =0; j< OUT_WIDTH; j++){
			#pragma HLS LOOP_TRIPCOUNT min=0 max=2
			#pragma HLS PIPELINE
			#pragma HLS UNROLL factor=2
			out_channel.write(buf2[i][j]);
		}
	}
	pos += ROW_WORKERS;
}


void process_channel(stream_t in_channel, stream_t out_channel){
	// Column worker takes new input for every new channel.
	c.initialize(in_channel);
	lanczosComputeBuffers:
	for(int i = 0; i < (OUT_HEIGHT+ROW_WORKERS-1)/ROW_WORKERS; i++){
		#pragma HLS DATAFLOW
		num_t buf1[IN_WIDTH][ROW_WORKERS];
#pragma HLS ARRAY_PARTITION variable=buf1 complete dim=2

		byte_t buf2[ROW_WORKERS][OUT_WIDTH];
#pragma HLS ARRAY_PARTITION variable=buf2 complete dim=1
		fillColBuffer(in_channel, buf1); // byte to num_t
		fillRowBuffer(buf1, buf2); // num_T to byte
		stream_out(buf2, out_channel);
	}
}


void lanczos(
	stream_t streamin,
	stream_t streamout
) {
//#pragma HLS STABLE variable=streamout
//#pragma HLS STABLE variable=streamin
//#pragma HLS STABLE variable=c
//#pragma HLS STABLE variable=r
#pragma HLS INTERFACE axis register both port=streamin
#pragma HLS INTERFACE axis register both port=streamout
	// Perform column lengthening first, then row lengthening
	process_channel(streamin, streamout);
}
