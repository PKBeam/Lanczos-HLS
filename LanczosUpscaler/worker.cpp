#include "worker.h"
#include "kernel.h"
// instantiate types in .h

ColWorkers::ColWorkers(int offset): offset(offset){
	curr_offset = offset;
}

void ColWorkers::exec(byte_t input[IN_HEIGHT][IN_WIDTH], kernel_t kern_vals[2*LANCZOS_A], num_t output[IN_WIDTH][ROW_WORKERS]){
	compute_loop:
    for(int i = 0; i < IN_WIDTH; i++){
        output[i][out_idx] = compute<byte_t>(input_buffers[i], kern_vals);
    }
    out_idx++;
    if ((out_idx-curr_offset)*SCALE_D >= (in_idx - LANCZOS_A)*SCALE_N) step_input(input);
}

void ColWorkers::step_input(byte_t input[IN_HEIGHT][IN_WIDTH]){
    for(int i = 0; i < IN_WIDTH; i++){
    	shift_down<byte_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_HEIGHT? (byte_t) 0 : input[in_idx][i]);
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
    for (int j = 0; j < 2*LANCZOS_A; j++){
        for(int i = 0; i < IN_WIDTH; i++){

            input_buffers[i][j] =  in_idx < 0 ? (byte_t) 0 :input[in_idx][i];
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
	compute_loop:
    for(int i = 0; i < ROW_WORKERS; i++){
        output[i][out_idx] = clamp_to_byte(compute<num_t>(input_buffers[i], kern_vals));
    }
    out_idx++;
    if ((out_idx-curr_offset)*SCALE_D >= (in_idx - LANCZOS_A)*SCALE_N) step_input(input);
}

void RowWorkers::step_input(num_t input[OUT_HEIGHT][ROW_WORKERS]){
    for(int i = 0; i < ROW_WORKERS; i++){

        shift_down<num_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_WIDTH? (num_t) 0 : input[in_idx][i]);
    }
    in_idx++;
}

void RowWorkers::initialize(num_t input[OUT_HEIGHT][ROW_WORKERS]){
    // clear and initialize buffer with first few values of input
    out_idx = 0;
    curr_offset = offset;
    // Get first value of in_idx
    in_idx = (-offset*SCALE_D)/SCALE_N - LANCZOS_A + 1; // integer division floors for me
    for (int j = 0; j < 2*LANCZOS_A; j++){
    	for(int i = 0; i < ROW_WORKERS; i++){

            input_buffers[i][j] =  in_idx < 0 ? (num_t) 0 :input[in_idx][i];
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


//void row_worker_alone(byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH], num_t img_out_ob[NUM_CHANNELS][OUT_HEIGHT][IN_WIDTH]){
//	ColWorkers c_worker(3);
//	for (int chan=0; chan < NUM_CHANNELS; chan++){
//
//		c_worker.initialize(img_in[chan]);
//		for(int i = 0; i < ROW_WORKERS; i++){
//			// get kernel values
//			kernel_t kernel_vals[2*LANCZOS_A];
//
//			for(int j=0; j < 2*LANCZOS_A; j++){
//
//				kernel_vals[j] = lanczos_kernel(c_worker.in_idx - 2*LANCZOS_A + j, c_worker.out_idx - c_worker.offset, (scale_t)SCALE);
//
//			}
//			c_worker.exec(img_in[chan], kernel_vals, img_out_ob[chan]);
//		}
//	}
//}
