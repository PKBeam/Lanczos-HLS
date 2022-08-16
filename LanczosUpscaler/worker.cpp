#include "worker.h"
#include "kernel.h"
#include "cyclic_buffer/cyclic_buffer.h"
// instantiate types in .h

// STREAM TODO
#include "hls_stream.h"


void unpack_blob(num_t blob, num_el_t out[NUM_CHANNELS]){
#pragma HLS INLINE
	for (int i = 0; i < NUM_CHANNELS; i++){
#pragma HLS UNROLL
		out[i]((INTEGER_BITS + BIT_PRECISION)-1,0) = blob((i+1)*(INTEGER_BITS + BIT_PRECISION)-1, i*(INTEGER_BITS + BIT_PRECISION));
	}
}

void unpack_blob(byte_t blob, byte_el_t out[NUM_CHANNELS]){
#pragma HLS INLINE
	for (int i = 0; i < NUM_CHANNELS; i++){
		#pragma HLS UNROLL
		out[i](7,0) = blob((i+1)*8-1, i*8);
	}
}

num_t pack_blob(num_el_t in[NUM_CHANNELS]){
#pragma HLS INLINE
	num_t blob;
	for (int i = 0; i < NUM_CHANNELS; i++){
		#pragma HLS UNROLL
		blob((i+1)*(INTEGER_BITS + BIT_PRECISION)-1, i*(INTEGER_BITS + BIT_PRECISION)) = in[i]((INTEGER_BITS + BIT_PRECISION)-1,0);
	}
	return blob;
}
byte_t pack_blob(byte_el_t in[NUM_CHANNELS]){
#pragma HLS INLINE
	byte_t blob;
	for (int i = 0; i < NUM_CHANNELS; i++){
		#pragma HLS UNROLL
		blob((i+1)*8-1, i*8) = in[i](7,0);
	}
	return blob;
}

num_t compute(cyclic_buffer_t::Slice &in, kernel_t kern[2*LANCZOS_A]){

	num_el_t acc[NUM_CHANNELS] = {0,0,0};
	#pragma HLS ARRAY_PARTITION complete variable=acc

	byte_el_t in_unpacked[2*LANCZOS_A][NUM_CHANNELS];
	#pragma HLS ARRAY_PARTITION complete variable=in_unpacked dim=1
	#pragma HLS ARRAY_PARTITION complete variable=in_unpacked dim=2

    for(int i = 0; i < 2*LANCZOS_A; i++){
    	unpack_blob(in[i], in_unpacked[i]);
    	for(int j = 0; j< NUM_CHANNELS ; j++){
    		#pragma HLS UNROLL
    		acc[j] += kern[i]*in_unpacked[i][j];
    	}
    }

    num_el_t out[NUM_CHANNELS];
#pragma HLS ARRAY_PARTITION variable=out complete dim=1
    for(int j = 0; j< NUM_CHANNELS ; j++){
		#pragma HLS UNROLL
		num_el_t min = MIN(in_unpacked[LANCZOS_A-1][j], in_unpacked[LANCZOS_A][j]);
		num_el_t max = MAX(in_unpacked[LANCZOS_A-1][j], in_unpacked[LANCZOS_A][j]);
		if (acc[j] < min) {
			out[j] = min;
		}else if (acc[j] > max) {
			out[j] = max;
		} else {
			out[j] = acc[j];
		}
	}

    return pack_blob(out);
}


num_t compute_(num_t in[2*LANCZOS_A], kernel_t kern[2*LANCZOS_A]){
	#pragma HLS INLINE

	num_el_t acc[NUM_CHANNELS] = {0,0,0};
	#pragma HLS ARRAY_PARTITION complete variable=acc

	num_el_t in_unpacked[2*LANCZOS_A][NUM_CHANNELS];
	#pragma HLS ARRAY_PARTITION complete variable=in_unpacked dim=1
	#pragma HLS ARRAY_PARTITION complete variable=in_unpacked dim=2

    for(int i = 0; i < 2*LANCZOS_A; i++){
    	unpack_blob(in[i], in_unpacked[i]);
    	for(int j = 0; j< NUM_CHANNELS ; j++){
    		#pragma HLS UNROLL
    		acc[j] += kern[i]*in_unpacked[i][j];
    	}
    }

    num_el_t out[NUM_CHANNELS];
#pragma HLS ARRAY_PARTITION variable=out complete dim=1
    for(int j = 0; j< NUM_CHANNELS ; j++){
		#pragma HLS UNROLL
		num_el_t min = MIN(in_unpacked[LANCZOS_A-1][j], in_unpacked[LANCZOS_A][j]);
		num_el_t max = MAX(in_unpacked[LANCZOS_A-1][j], in_unpacked[LANCZOS_A][j]);
		if (acc[j] < min) {
			out[j] = min;
		}else if (acc[j] > max) {
			out[j] = max;
		} else {
			out[j] = acc[j];
		}
	}

    return pack_blob(out);
}

// Perform local clamping after whole claculation. check that this actually works. Designed for num_t = ap_fixed<X,9>
byte_t clamp_to_byte(num_t x){
	num_el_t x_unpacked[NUM_CHANNELS];
	byte_el_t out_unpacked[NUM_CHANNELS];
	unpack_blob(x, x_unpacked);

	#pragma HLS ARRAY_PARTITION variable=out_unpacked complete dim=1
	#pragma HLS ARRAY_PARTITION variable=x_unpacked complete dim=1
	for(int j =0; j<NUM_CHANNELS;j++){
		out_unpacked[j] = byte_el_t(x_unpacked[j]);
	}

	return pack_blob(out_unpacked);
}



ColWorkers::ColWorkers(col_major_counter_t offset): offset(offset){
	curr_offset = offset;
}

void ColWorkers::exec(stream_t input, kernel_t kern_vals[2*LANCZOS_A], num_t output[IN_WIDTH][ROW_WORKERS]){

    bool step_cond = fractional_t(num_el_t(1/SCALE)*(out_idx+curr_offset+1)) < fractional_t(1/SCALE);
	col_compute_loop:
	for(col_minor_counter_t i = 0; i < IN_WIDTH; i++){
		#pragma HLS DEPENDENCE variable=input_buffers[i] array intra WAR false
		#pragma HLS DEPENDENCE variable=input_buffers[i] array intra RAW false
		#pragma HLS PIPELINE
        output[i][out_idx] = compute(input_buffers[i], kern_vals);
    	input_buffers.buffer_write(i, (in_idx < IN_HEIGHT && step_cond)? input.read() : (byte_t) 0);
        //output[i][out_idx] = compute<byte_t>(input_buffers[i], kern_vals);
    }
	if(step_cond){
	    input_buffers.push(in_idx >= IN_HEIGHT); // push with saturate if we exceed
		in_idx ++;
	}
    out_idx++;
}


//void ColWorkers::step_input(stream_t input){
//	colWorkers_stepBuffers:
//    for(col_minor_counter_t i = 0; i < IN_WIDTH; i++){
//		#pragma HLS PIPELINE
//    	input_buffers.buffer_write(i, in_idx >= IN_HEIGHT? (byte_t) 0: input.read());
//    	//shift_down<byte_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_HEIGHT? (byte_t) 0 : input[in_idx][i]);
//    }
//    input_buffers.push(in_idx >= IN_HEIGHT); // push with saturate if we exceed
//    in_idx++;
//}


void ColWorkers::initialize(stream_t input){
    // clear and initialize buffer with first few values of input
    out_idx = 0;
    curr_offset = offset;
    // Get first value of in_idx

    const int N_ZEROS = LANCZOS_A - (offset*SCALE_D)/SCALE_N - 1;

    colWorkers_zeros:
    for (int j = 0; j < LANCZOS_A*2; j++){
    	rowWorkers_zeros_inner:
    	for(col_minor_counter_t i = 0; i < IN_WIDTH; i++){
#pragma HLS PIPELINE
    		input_buffers.buffer_write(i,  j < N_ZEROS ? (byte_t)0 : (byte_t)input.read());
//            input_buffers[i][j] =  (num_t) 0;
        }
    	input_buffers.push(false); // push without saturate
    }
    in_idx = LANCZOS_A*2 - N_ZEROS;
//    colWorkers_init:
//    for (int j = 0; j < 2*LANCZOS_A - N_ZEROS; j++){
//    	rowWorkers_init_inner:
//    	for(col_minor_counter_t i = 0; i < IN_WIDTH; i++){
//    		input_buffers[i].set(j + N_ZEROS, input.read());
////            input_buffers[i][j] = input[in_idx][i];
//        }
//    	in_idx ++;
//    }
}
void ColWorkers::seek_write_index(col_major_counter_t idx){
	curr_offset += out_idx - idx; // maintain offset so that
	out_idx = idx;
}

col_major_counter_t ColWorkers::get_out_pos(){
	return out_idx + curr_offset;
}



template <typename T, int N>
T shift_down(T reg[N], T next){
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

void RowWorkers::exec(num_t input[IN_WIDTH][ROW_WORKERS], kernel_t kern_vals[2*LANCZOS_A], byte_t output[ROW_WORKERS][OUT_WIDTH]){
	#pragma HLS ALLOCATION instances=Mul limit=1 core
	#pragma HLS INLINE // inlines help get II to 1 in this case
	row_compute_loop:
	for(row_minor_counter_t i = 0; i < ROW_WORKERS;i ++){
		#pragma HLS UNROLL
		output[i][out_idx] = clamp_to_byte(compute_(input_buffers[i], kern_vals));
	}
	out_idx++;
	if (fractional_t(num_el_t(1/SCALE)*(out_idx+curr_offset)) < fractional_t(1/SCALE)) step_input(input);

}


void RowWorkers::step_input(num_t input[IN_WIDTH][ROW_WORKERS]){
	#pragma HLS INLINE // inlines help get II to 1 in this case
	rowWorkers_stepBuffers:
    for(row_minor_counter_t i = 0; i < ROW_WORKERS; i++){
		#pragma HLS UNROLL
    	shift_down<num_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_WIDTH? (num_t)input_buffers[i][LANCZOS_A*2-1] : (num_t)input[in_idx][i]);
    }
	in_idx++;
}

void RowWorkers::initialize(num_t input[IN_WIDTH][ROW_WORKERS]){
    // clear and initialize buffer with first few values of input
    out_idx = 0;
    curr_offset = offset;
    // Get first value of in_idx
    in_idx = 0; // integer division floors for me

    const int N_ZEROS = LANCZOS_A - (offset*SCALE_D)/SCALE_N - 1;
    rowWorkers_zeros:
    for (int j = 0; j < LANCZOS_A*2; j++){
#pragma HLS UNROLL
    	rowWorkers_zeros_inner:
		for(row_minor_counter_t i = 0; i < ROW_WORKERS; i++){
			input_buffers[i][j] =  j < N_ZEROS ?  (num_t)0 : input[in_idx][i];
		}
		if (j >= N_ZEROS) in_idx++;
    }
//    rowWorkers_init:
//    for (int j = 0; j < 2*LANCZOS_A - N_ZEROS; j++){
//    	rowWorkers_init_inner:
//    	for(row_minor_counter_t i = 0; i < ROW_WORKERS; i++){
////    		input_buffers[i].set(j, in_idx < 0 ? (num_t) 0 :input[in_idx][i]);
//            input_buffers[i][j + N_ZEROS] = ;
//        }
//    	in_idx ++;
//    }
}

void RowWorkers::seek_write_index(row_major_counter_t idx){
	curr_offset += out_idx - idx; // maintain offset so that
	out_idx = idx;
}

row_major_counter_t RowWorkers::get_out_pos(){
	return out_idx + curr_offset;
}
