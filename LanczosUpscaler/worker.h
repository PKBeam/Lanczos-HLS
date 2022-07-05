#include "lanczos.h"

// worker usage be like:

/*
-> take in entire input row
-> Spit out entire output row


worker(array, )

issue of course is the synchronization. Can this be done by HLS?

want to basically be able to do

col = worker(row, );
 

As long as col is required abstracted to memory somehow, this is really O.K.

Sure, the output values might come out as a stream, but at the end of the day, can we really use them?

if we assume no input duplication and no multitasking, i.e the workers run to completion every time, and also assume that there is no input buffer
duplication, we have that at least IN_WIDTH*IN_HEIGHT*SCALE*NUM_WIDTH bits stored in memory. This is a bit bad.

If we assume regular monitor resolution, we already exceed 2 megapixels. This is a bad

OK then, lets say that we partition inputs and store those buffers. In order to do that, we need at IN_HEIGHT*2A*NUM_WIDTH bits. This can be around 



. . . . . . . .                                   .............
. . . .
. . . .
. . . .
. . . .



. . . .



. . . .



. . . .



. . . .



. . . .


So we are doing input partitioning. 

How should the partitioning be achieved?

- Take in entire input COLUMN.

- HLS should do the monkey work for scheduling multiplications itself


now the worker is spitting out entire columns at "a time". 



ROW WORKER

Just because of of number of rows required, a row worker will need to mux all the input values that are relevant.


COLUMN WORKER

A column worker (in a fixed configuration) will not really get a choice of where it soruces its data from.

When specifying the data source to the column worker it needs to be buffered from the outputs of the row column.
Generally we can achieve this by using variables or completely partitioned arrays.


worker (input, int col, next_signal, kernel_vals){
    for each row in input
        if next_signal:    
            row.buffer.push(row[col])
        worker(row.buffer, kernel_vals)

}

Complete partition workers




*/



// Perform local clamping after whole claculation. check that this actually works. Designed for num_t = ap_fixed<X,9>
byte_t clamp_to_byte(num_t x){
	if (x[BIT_PRECISION+8]){
		return x[BIT_PRECISION+7] ? 0 : 255;
	} else {
		return x;
	}
}

template <typename T, int N>
T shift_up(T reg[N], T next){
    T out = reg[N-1];
    for (k = N-2; k >= 0; k--) reg[k] = reg[k-1];
    reg[0] = next;
    return out;
}


template <typename T, int N>
T shift_down(T reg[N], T next){
    T out = reg[0];
    for (k = 0; k < N-1; k++) reg[k] = reg[k+1];
    reg[N-1] = next;
    return out;
}


template <typename IN_T> num_t compute(IN_T[2*LANCZOS_A], kernel_t[2*LANCZOS_A]);
template <typename IN_T>
num_t compute(IN_T[2*LANCZOS_A] in, kernel_t[2*LANCZOS_A] kern){
    num_t out = 0;
    for(int i = 0; i < 2*LANCZOS_A; i++){
        out += kern[i]*in[i];
    }
    return out;
}


template <int N, int IN_LEN, int OUT_LEN, int OFFSET>
class proc {
private:
public:
    
    // One buffer per input row, each buffer is 2A in width
    byte_t input_buffers[N][LANCZOS_A*2];

    // internally maintained counters. Treat these as read only!!
    // Note:
    // in_idx is the NEXT index to read from.
    // out_idx is the NEXT index to write to. 
    //
    // Because out_idx requires entries ahead to be read, the correspoinding position
    // in the input image that out_idx corresponds to is actually in the interval (in_idx - OFFSET - 1, in_idx - OFFSET]
    //
    // So we want out_idx/scale <= in_idx - OFFSET.
    // This constraint translates to out_idx*scale_D <= (in_idx - OFFSET)*scale_N

    int in_idx = 0; 
    int out_idx = 0;
    
    // Control logic to stop executing will be provided externally.
    void exec(byte_t input[N][IN_LEN], kernel_t[2*LANCZOS_A] kern_vals, num_t output[OUT_LEN][IN_LEN]){
        for(int i = 0; i < N; i++){
            output[out_idx][i] = compute<byte_t>(input_buffers[i], kern_vals);
        }
        if (out_idx*SCALE_D >= (in_idx - OFFSET)*SCALE_N) step_input(input);
        out_idx++;
    }

    void step_input(byte_t input[N][IN_LEN]){
        for(int i = 0; i < N; i++){
            shift_up<byte_t, 2*LANCZOS_A>(input_buffers[i], input[i][in_idx]);
        }
        in_idx++;
    }

    void initialize(byte_t input[N][IN_LEN]){
        // clear and initialize buffer with first few values of input
        out_idx = 0;
        for (in_idx = -OFFSET; in_idx < LANCZOS_A*2 - OFFSET; in_idx++){
            for(int i = 0; i < N; i++){
                #pragma HLS UNROLL complete
                input_buffers[i][j] =  in_idx < 0 ? (byte_t) 0 :input[i][in_idx];
            }
        }
    }
}
