#include "worker.h"
#include "kernel.h"
#include "cyclic_buffer/cyclic_buffer.h"
// instantiate types in .h

/*
template <typename T, int N>
T shift_up(T reg[N], T next){
    T out = reg[N-1];
    for (int k = N-1; k > 0; k--) reg[k] = reg[k-1];
    reg[0] = next;
    return out;
}

template <typename T, int N>
T shift_down(T reg[N], T next){
    T out = reg[0];
    for (int k = 0; k < N-1; k++) reg[k] = reg[k+1];
    reg[N-1] = next;
    return out;
}


template <typename IN_T>
num_t compute(IN_T in[2*LANCZOS_A], kernel_t kern[2*LANCZOS_A]){
    num_t out = 0;
    for(int i = 0; i < 2*LANCZOS_A; i++){
		#pragma HLS UNROLL
    	out += kern[i]*in[i];
    }
    return out;
}

*/
template <typename IN_T>
num_t compute(CyclicBuffer<IN_T, ap_uint<2>, LANCZOS_A*2> in, kernel_t kern[2*LANCZOS_A]){
    num_t out = 0;
    for(int i = 0; i < 2*LANCZOS_A; i++){
		#pragma HLS UNROLL
    	out += kern[i]*in.get(i);
    }
    return out;
}

ColWorkers::ColWorkers(int offset): offset(offset){
	curr_offset = offset;
}

void ColWorkers::exec(byte_t input[IN_HEIGHT][IN_WIDTH], kernel_t kern_vals[2*LANCZOS_A], num_t output[IN_WIDTH][ROW_WORKERS]){
	col_compute_loop:
    for(int i = 0; i < IN_WIDTH; i++){
		#pragma HLS PIPELINE
        output[i][out_idx] = compute<byte_t>(input_buffers[i], kern_vals);
        //output[i][out_idx] = compute<byte_t>(input_buffers[i], kern_vals);
    }
    out_idx++;
    if ((out_idx-curr_offset)*SCALE_D >= (in_idx - LANCZOS_A)*SCALE_N) step_input(input);
}

void ColWorkers::step_input(byte_t input[IN_HEIGHT][IN_WIDTH]){
	colWorkers_stepBuffers:
    for(int i = 0; i < IN_WIDTH; i++){
		#pragma HLS PIPELINE
    	input_buffers[i].shift_left(in_idx >= IN_HEIGHT? (byte_t) 0 : input[in_idx][i]);
    	//shift_down<byte_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_HEIGHT? (byte_t) 0 : input[in_idx][i]);
    }
    in_idx++;
}

void ColWorkers::initialize(byte_t input[IN_HEIGHT][IN_WIDTH]){
    // clear and initialize buffer with first few values of input

	// want that in_idx - 1 <= (out_idx-offset)/scale + LANCZOS_A < in_idx  upon calculation
    out_idx = 0;
    curr_offset = offset;
    // Get first value of in_idx
    in_idx = (-offset*SCALE_D)/SCALE_N - LANCZOS_A + 1; // integer division floors for me
    colWorkers_init:
    for (int j = 0; j < 2*LANCZOS_A; j++){
    	colWorkers_init_width:
        for(int i = 0; i < IN_WIDTH; i++){
        	input_buffers[i].set(j, in_idx < 0 ? (byte_t) 0 :input[in_idx][i]);
            //input_buffers[i][j] =  in_idx < 0 ? (byte_t) 0 :input[in_idx][i];
        }
        in_idx ++;
    }
}

void ColWorkers::seek_write_index(int idx){
	curr_offset += idx - out_idx; // maintain offset so that
	out_idx = idx;
}

int ColWorkers::get_out_pos(){
	return out_idx - curr_offset;
}



// Perform local clamping after whole claculation. check that this actually works. Designed for num_t = ap_fixed<X,9>
byte_t clamp_to_byte(num_t x){
//	if (x[BIT_PRECISION+8]){
//		return x[BIT_PRECISION+7] ? 0 : 255;
//	} else {
//		return x;
//	}
	if (x>255){
		return 255;
	}
	if (x < 0){
		return 0;
	}
	return x;
}


RowWorkers::RowWorkers(int offset): offset(offset){
	curr_offset = offset;
}

void RowWorkers::exec(num_t input[OUT_HEIGHT][ROW_WORKERS], kernel_t kern_vals[2*LANCZOS_A], byte_t output[ROW_WORKERS][OUT_WIDTH]){
	row_compute_loop:
    for(int i = 0; i < ROW_WORKERS; i++){
		#pragma HLS PIPELINE
        output[i][out_idx] = clamp_to_byte(compute(input_buffers[i], kern_vals));
        //output[i][out_idx] = clamp_to_byte(compute<num_t>(input_buffers[i], kern_vals));
    }
    out_idx++;
    if ((out_idx-curr_offset)*SCALE_D >= (in_idx - LANCZOS_A)*SCALE_N) step_input(input);
}

void RowWorkers::step_input(num_t input[OUT_HEIGHT][ROW_WORKERS]){
	rowWorkers_stepBuffers:
    for(int i = 0; i < ROW_WORKERS; i++){
		#pragma HLS PIPELINE
    	input_buffers[i].shift_left(in_idx >= IN_WIDTH? (num_t) 0 : input[in_idx][i]);
        //shift_down<num_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_WIDTH? (num_t) 0 : input[in_idx][i]);
    }
    in_idx++;
}

void RowWorkers::initialize(num_t input[OUT_HEIGHT][ROW_WORKERS]){
    // clear and initialize buffer with first few values of input
    out_idx = 0;
    curr_offset = offset;
    // Get first value of in_idx
    in_idx = (-offset*SCALE_D)/SCALE_N - LANCZOS_A + 1; // integer division floors for me
    rowWorkers_init:
    for (int j = 0; j < 2*LANCZOS_A; j++){
    	rowWorkers_init_inner:
    	for(int i = 0; i < ROW_WORKERS; i++){
    		input_buffers[i].set(j, in_idx < 0 ? (num_t) 0 :input[in_idx][i]);
            //input_buffers[i][j] =  in_idx < 0 ? (num_t) 0 :input[in_idx][i];
        }
    	in_idx ++;
    }
}

void RowWorkers::seek_write_index(int idx){
	curr_offset += idx - out_idx; // maintain offset so that
	out_idx = idx;
}

int RowWorkers::get_out_pos(){
	return out_idx - curr_offset;
}
