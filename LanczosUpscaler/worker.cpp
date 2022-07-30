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
//template <typename IN_T>
//num_t compute(CyclicBuffer<IN_T, ap_uint<2>, LANCZOS_A*2> &in, kernel_t kern[2*LANCZOS_A]){
//    num_t out = 0;
//    for(int i = 0; i < 2*LANCZOS_A; i++){
//		#pragma HLS UNROLL
//    	out += kern[i]*in.get(i);
//    }
//    return out;
//}


num_t compute(cyclic_buffer_t &in, kernel_t kern[2*LANCZOS_A]){
	num_el_t acc[NUM_CHANNELS] = {0,0,0};
#pragma HLS ARRAY_PARTITION complete variable=acc
    for(int i = 0; i < 2*LANCZOS_A; i++){
		#pragma HLS UNROLL
    	for(int j = 0; j< NUM_CHANNELS ; j++){
    		#pragma HLS UNROLL
    		acc[j] += kern[i]*in.get(i)[j];
    	}
    }

    num_t out;
    for(int j = 0; j< NUM_CHANNELS ; j++){
		#pragma HLS UNROLL
		num_el_t min = (num_el_t)(in.get(LANCZOS_A-1)[j] < in.get(LANCZOS_A)[j] ? in.get(LANCZOS_A-1)[j] : in.get(LANCZOS_A)[j]);
		num_el_t max = (num_el_t)(in.get(LANCZOS_A-1)[j] > in.get(LANCZOS_A)[j] ? in.get(LANCZOS_A-1)[j] : in.get(LANCZOS_A)[j]);
		if (acc[j] < min) {
			out.write(j, min);
		}else if (acc[j] > max) {
			out.write(j, max);
		} else {
			out.write(j, acc[j]);
		}

	}
    return out;
}

num_t compute_(num_t in[2*LANCZOS_A], kernel_t kern[2*LANCZOS_A]){
	#pragma HLS INLINE
	num_el_t acc[NUM_CHANNELS] = {0,0,0};
	#pragma HLS ARRAY_PARTITION complete variable=acc
    for(int i = 0; i < 2*LANCZOS_A; i++){
    	for(int j = 0; j< NUM_CHANNELS ; j++){
			#pragma HLS UNROLL
    		acc[j] += kern[i]*in[i][j];
    	}
    }

    num_t out;
    for(int j = 0; j< NUM_CHANNELS ; j++){
		#pragma HLS UNROLL
		num_el_t min = in[LANCZOS_A-1][j] < in[LANCZOS_A][j] ? in[LANCZOS_A-1][j] : in[LANCZOS_A][j];
		num_el_t max = in[LANCZOS_A-1][j] > in[LANCZOS_A][j] ? in[LANCZOS_A-1][j] : in[LANCZOS_A][j];
		if (acc[j] < min) {
			out.write(j, min);
		}else if (acc[j] > max) {
			out.write(j, max);
		} else {
			out.write(j, acc[j]);
		}
	}
    return out;
}

// Perform local clamping after whole claculation. check that this actually works. Designed for num_t = ap_fixed<X,9>
byte_t clamp_to_byte(num_t x){
//	if (x[BIT_PRECISION+8]){
//		return x[BIT_PRECISION+7] ? 0 : 255;
//	} else {
//		return x;
//	}
	byte_t out;
	for(int j =0; j<NUM_CHANNELS;j++){
		num_el_t item = x[j];
		if (item > 255){
			out.write(j, 255);
		}else if (item < 0){
			out.write(j, 0);
		}else{
			out.write(j, byte_el_t(item));
		}
	}
	return out;
}



ColWorkers::ColWorkers(col_major_counter_t offset): offset(offset){
	curr_offset = offset;
}


void ColWorkers::exec(byte_t input[IN_HEIGHT][IN_WIDTH], kernel_t kern_vals[2*LANCZOS_A], num_t output[IN_WIDTH][ROW_WORKERS]){
	col_compute_loop:
    for(col_minor_counter_t i = 0; i < IN_WIDTH; i++){
		#pragma HLS PIPELINE
        output[i][out_idx] = compute(input_buffers[i], kern_vals);
        //output[i][out_idx] = compute<byte_t>(input_buffers[i], kern_vals);
    }
    out_idx++;
    if ((out_idx+curr_offset)*SCALE_D + LANCZOS_A*SCALE_N >= in_idx*SCALE_N) step_input(input);
}

void ColWorkers::step_input(byte_t input[IN_HEIGHT][IN_WIDTH]){
	colWorkers_stepBuffers:
    for(col_minor_counter_t i = 0; i < IN_WIDTH; i++){
		#pragma HLS PIPELINE
    	byte_t new_val = in_idx >= IN_HEIGHT? (byte_t) 0 : input[in_idx][i];
    	input_buffers[i].shift_left(new_val);
    	//shift_down<byte_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_HEIGHT? (byte_t) 0 : input[in_idx][i]);
    }
    in_idx++;
}

void ColWorkers::initialize(byte_t input[IN_HEIGHT][IN_WIDTH]){
    // clear and initialize buffer with first few values of input
    out_idx = 0;
    curr_offset = offset;
    // Get first value of in_idx
    in_idx = (offset*SCALE_D)/SCALE_N - LANCZOS_A + 1; // integer division floors for me

    const int N_ZEROS = LANCZOS_A - (offset*SCALE_D)/SCALE_N - 1;
    colWorkers_zeros:
    for (int j = 0; j < N_ZEROS; j++){
    	rowWorkers_zeros_inner:
    	for(col_minor_counter_t i = 0; i < IN_WIDTH; i++){
    		input_buffers[i].set(j, (byte_t)0);
//            input_buffers[i][j] =  (num_t) 0;
        }
    	in_idx ++;
    }
    colWorkers_init:
    for (int j = 0; j < 2*LANCZOS_A - N_ZEROS; j++){
    	rowWorkers_init_inner:
    	for(col_minor_counter_t i = 0; i < IN_WIDTH; i++){
    		input_buffers[i].set(j + N_ZEROS, input[in_idx][i]);
//            input_buffers[i][j] = input[in_idx][i];
        }
    	in_idx ++;
    }
}

void ColWorkers::seek_write_index(col_major_counter_t idx){
	curr_offset += out_idx - idx; // maintain offset so that
	out_idx = idx;
}

col_major_counter_t ColWorkers::get_out_pos(){
	return out_idx + curr_offset;
}



template <typename T, int N>
T shift_down(T reg[N], T& next){
    T out = reg[0];
    for (int k = 0; k < N-1; k++) reg[k] = reg[k+1];
    reg[N-1] = next;
    return out;
}

RowWorkers::RowWorkers(row_major_counter_t offset): offset(offset){
	curr_offset = offset;
#pragma HLS ARRAY_PARTITION variable=input_buffers complete dim=1
#pragma HLS ARRAY_PARTITION variable=input_buffers complete dim=2
}

void RowWorkers::exec(num_t input[OUT_HEIGHT][ROW_WORKERS], kernel_t kern_vals[2*LANCZOS_A], byte_t output[ROW_WORKERS][OUT_WIDTH]){
	#pragma HLS ALLOCATION instances=Mul limit=1 core
	#pragma HLS INLINE // inlines help get II to 1 in this case
	row_compute_loop:
    for(row_minor_counter_t i = 0; i < ROW_WORKERS; i++){
//        output[i][out_idx] = clamp_to_byte(compute(input_buffers[i], kern_vals));
        output[i][out_idx] = clamp_to_byte(compute_(input_buffers[i], kern_vals));
    }
    out_idx++;
    if ((out_idx+curr_offset)*SCALE_D + LANCZOS_A*SCALE_N >= in_idx*SCALE_N) step_input(input);
}

void RowWorkers::step_input(num_t input[OUT_HEIGHT][ROW_WORKERS]){
#pragma HLS INLINE // inlines help get II to 1 in this case
	rowWorkers_stepBuffers:
    for(row_minor_counter_t i = 0; i < ROW_WORKERS; i++){
		#pragma HLS PIPELINE
//    	input_buffers[i].shift_left(in_idx >= IN_WIDTH? (num_t) 0 : input[in_idx][i]);
    	num_t new_val = in_idx >= IN_WIDTH? input_buffers[i][LANCZOS_A*2-1] : input[in_idx][i];
        shift_down<num_t, 2*LANCZOS_A>(input_buffers[i], new_val);
    }
    in_idx++;
}

void RowWorkers::initialize(num_t input[OUT_HEIGHT][ROW_WORKERS]){
    // clear and initialize buffer with first few values of input
    out_idx = 0;
    curr_offset = offset;
    // Get first value of in_idx
    in_idx = (offset*SCALE_D)/SCALE_N - LANCZOS_A + 1; // integer division floors for me

    const int N_ZEROS = LANCZOS_A - (offset*SCALE_D)/SCALE_N - 1;
    rowWorkers_zeros:
    for (int j = 0; j < N_ZEROS; j++){
    	rowWorkers_zeros_inner:
    	for(row_minor_counter_t i = 0; i < ROW_WORKERS; i++){
//    		input_buffers[i].set(j, in_idx < 0 ? (num_t) 0 :input[in_idx][i]);
            input_buffers[i][j] =  (num_t) 0;
        }
    	in_idx ++;
    }
    rowWorkers_init:
    for (int j = 0; j < 2*LANCZOS_A - N_ZEROS; j++){
    	rowWorkers_init_inner:
    	for(row_minor_counter_t i = 0; i < ROW_WORKERS; i++){
//    		input_buffers[i].set(j, in_idx < 0 ? (num_t) 0 :input[in_idx][i]);
            input_buffers[i][j + N_ZEROS] = input[in_idx][i];
        }
    	in_idx ++;
    }
}

void RowWorkers::seek_write_index(row_major_counter_t idx){
	curr_offset += out_idx - idx; // maintain offset so that
	out_idx = idx;
}

row_major_counter_t RowWorkers::get_out_pos(){
	return out_idx + curr_offset;
}
