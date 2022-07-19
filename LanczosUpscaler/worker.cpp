#include "worker.h"
#include "kernel.h"
// instantiate types in .h

RowWorker::RowWorker(int offset): offset(offset){}

void RowWorker::exec(byte_t input[IN_HEIGHT][IN_WIDTH], kernel_t kern_vals[2*LANCZOS_A], num_t output[COL_WORKERS][IN_HEIGHT]){
	compute_loop:
    for(int i = 0; i < IN_HEIGHT; i++){
        output[out_idx][i] = compute<byte_t>(input_buffers[i], kern_vals);
    }
    out_idx++;
    if ((out_idx-offset)*SCALE_D >= (in_idx - LANCZOS_A)*SCALE_N) step_input(input);
}

void RowWorker::step_input(byte_t input[IN_HEIGHT][IN_WIDTH]){
    for(int i = 0; i < IN_HEIGHT; i++){

    	shift_down<byte_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_WIDTH? (byte_t) 0 : input[i][in_idx]);
    }
    in_idx++;
}

void RowWorker::initialize(byte_t input[IN_HEIGHT][IN_WIDTH]){
    // clear and initialize buffer with first few values of input

	// want that in_idx - 1 <= (out_idx-offset)/scale + LANCZOS_A < in_idx  upon calculation
    out_idx = 0;
    // Get first value of in_idx
    in_idx = (-offset*SCALE_D)/SCALE_N - LANCZOS_A + 1; // integer division floors for me
    for (int j = 0; j < 2*LANCZOS_A; j++){
        for(int i = 0; i < IN_HEIGHT; i++){

            input_buffers[i][j] =  in_idx < 0 ? (byte_t) 0 :input[i][in_idx];
        }
        in_idx ++;
    }
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


ColWorker::ColWorker(int offset): offset(offset){}

void ColWorker::exec(num_t input[COL_WORKERS][IN_HEIGHT], kernel_t kern_vals[2*LANCZOS_A], byte_t output[COL_WORKERS][OUT_HEIGHT]){


	compute_loop:
    for(int i = 0; i < COL_WORKERS; i++){

        output[i][out_idx] = clamp_to_byte(compute<num_t>(input_buffers[i], kern_vals));
    }
    out_idx++;
    if ((out_idx-offset)*SCALE_D >= (in_idx - LANCZOS_A)*SCALE_N) step_input(input);
}

void ColWorker::step_input(num_t input[COL_WORKERS][IN_HEIGHT]){
    for(int i = 0; i < COL_WORKERS; i++){

        shift_down<num_t, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_HEIGHT? (num_t) 0 : input[i][in_idx]);
    }
    in_idx++;
}

void ColWorker::initialize(num_t input[COL_WORKERS][IN_HEIGHT]){
    // clear and initialize buffer with first few values of input
    out_idx = 0;
    // Get first value of in_idx
    in_idx = (-offset*SCALE_D)/SCALE_N - LANCZOS_A + 1; // integer division floors for me
    for (int j = 0; j < 2*LANCZOS_A; j++){
    	for(int i = 0; i < COL_WORKERS; i++){

            input_buffers[i][j] =  in_idx < 0 ? (num_t) 0 :input[i][in_idx];
        }
    	in_idx ++;
    }
}


void row_worker_alone(byte_t img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH], num_t img_out_ob[NUM_CHANNELS][COL_WORKERS][IN_HEIGHT]){
	RowWorker r_worker(3);
	for (int chan=0; chan< NUM_CHANNELS; chan++){

		r_worker.initialize(img_in[chan]);
		for(int i = 0; i < COL_WORKERS; i++){
			// get kernel values
			kernel_t kernel_vals[2*LANCZOS_A];

			for(int j=0; j < 2*LANCZOS_A; j++){

				kernel_vals[j] = lanczos_kernel(r_worker.in_idx - 2*LANCZOS_A + j, r_worker.out_idx - r_worker.offset, (scale_t)SCALE);

			}
			r_worker.exec(img_in[chan], kernel_vals, img_out_ob[chan]);
		}
	}
}
