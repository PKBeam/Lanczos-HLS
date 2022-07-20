#include "lanczos.h"
#ifndef WORKER_H
#define WORKER_H
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


.   .   .   .
. . . . . . .

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



RowWorker r(IN_HEIGHT);
ColWorker c(N);

BUF_READ[IN_HEIGHT][N]
BUF_WRITE[IN_HEIGHT][N]

fillBuffer(input, BUF_WRITE, r) // byte to num_t
for(i = 0; i < ceil(OUT_WIDTH/N) - 1; i++){
	BUF_WRITE, BUF_READ = BUF_READ, BUF_WRITE
	// col workers work on buf_read -> row worker work on input -> BUF_WRTE
	fillBuffer(input, BUF_WRITE, r) // byte to num_t
	fillBuffer(BUF_READ, output, c) // num_T to byte
}

fillBuffer(BUF_READ, output, c) // num_T to byte

fillBuffer(img, buf, proc)
	kern = get kernel values
	for i in range(N):
		proc.exec(img, kern, buf)





*/

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

class ColWorkers{
public:
	// offset = 0 when first out_idx is at the first pixel of the image.
	// want that in_idx - 1 < (out_idx-offset)/scale + LANCZOS_A <= in_idx  upon calculation
	const int offset;
	int curr_offset;
    // One buffer per input row, each buffer is23 2A in width
	byte_t input_buffers[IN_WIDTH][LANCZOS_A*2];


    // internally maintained counters. Treat these as read only!!
    // Note:
    // in_idx is the NEXT index to read from.
    // out_idx is the NEXT index to write to.
    //
    // Because out_idx requires entries ahead to be read, the correspoinding position
    // in the input image that out_idx corresponds to is actually in the interval (in_idx - offset - 1, in_idx - offset]
    //
    // So we want out_idx/scale <= in_idx - offset.
    // This constraint translates to out_idx*scale_D <= (in_idx - offset)*scale_N

    int in_idx = 0;
    int out_idx = 0;

    ColWorkers(int offset);
    void exec(byte_t[IN_HEIGHT][IN_WIDTH], kernel_t[2*LANCZOS_A], num_t[IN_WIDTH][ROW_WORKERS]);
    void step_input(byte_t[IN_HEIGHT][IN_WIDTH]);
    void initialize(byte_t[IN_HEIGHT][IN_WIDTH]);
    // out pos and write index are different: write index is the pointer offset. Out pos is the position of the
    // pixel being written to after the input image is upscaled by SCALE.
    void seek_write_index(int idx);
    int get_out_pos();

};

class RowWorkers{
public:
	// How many empty inputs there are upon initialize.
	// This translates to how shifted the image is after we perform the operation.
    const int offset;
	int curr_offset;

    // One buffer per input row, each buffer is 2A in width
	num_t input_buffers[ROW_WORKERS][LANCZOS_A*2];

	// internally maintained counters. Treat these as read only!!
    // Note:
    // in_idx is the NEXT index to read from.
    // out_idx is the NEXT index to write to.
    //
    // Because out_idx requires entries ahead to be read, the correspoinding position
    // in the input image that out_idx corresponds to is actually in the interval (in_idx - offset - 1, in_idx - offset]
    //
    // So we want out_idx/scale <= in_idx - offset.
    // This constraint translates to out_idx*scale_D <= (in_idx - offset)*scale_N

    int in_idx = 0;
    int out_idx = 0;

    // Control logic to stop executing will be provided externally.
    RowWorkers(int offset);
    void exec(num_t[IN_WIDTH][ROW_WORKERS], kernel_t[2*LANCZOS_A], byte_t[ROW_WORKERS][OUT_WIDTH]);
    void step_input(num_t[IN_WIDTH][ROW_WORKERS]);
    void initialize(num_t[IN_WIDTH][ROW_WORKERS]);
    // out pos and write index are different: write index is the pointer offset. Out pos is the position of the
    // pixel being written to after the input image is upscaled by SCALE.
    void seek_write_index(int idx);
    int get_out_pos();
};

#endif



// Template implementation for reference
//template <typename IN_T, typename OUT_T, int N, int IN_LEN, int OUT_LEN, int OFFSET>
//class Proc {
//public:
//
//    // One buffer per input row, each buffer is 2A in width
//	IN_T input_buffers[N][LANCZOS_A*2];
//
//    // internally maintained counters. Treat these as read only!!
//    // Note:
//    // in_idx is the NEXT index to read from.
//    // out_idx is the NEXT index to write to.
//    //
//    // Because out_idx requires entries ahead to be read, the correspoinding position
//    // in the input image that out_idx corresponds to is actually in the interval (in_idx - OFFSET - 1, in_idx - OFFSET]
//    //
//    // So we want out_idx/scale <= in_idx - OFFSET.
//    // This constraint translates to out_idx*scale_D <= (in_idx - OFFSET)*scale_N
//
//    int in_idx = 0;
//    int out_idx = 0;
//
//    // Control logic to stop executing will be provided externally.
//
//    void exec(IN_T[N][IN_LEN], kernel_t[2*LANCZOS_A], OUT_T[OUT_LEN][N]);
//    void step_input(IN_T[N][IN_LEN]);
//    void initialize(IN_T[N][IN_LEN]);
//};
//template <typename IN_T, typename OUT_T, int N, int IN_LEN, int OUT_LEN, int OFFSET>
//void Proc<IN_T, OUT_T, N, IN_LEN, OUT_LEN, OFFSET>::exec(IN_T input[N][IN_LEN], kernel_t kern_vals[2*LANCZOS_A], OUT_T output[OUT_LEN][N]){
//
//	#pragma HLS ARRAY_PARTITION variable=p.input_buffers cyclic factor=2 dim=1
//	#pragma HLS ARRAY_PARTITION variable=p.input_buffers complete dim=2
//
//	#pragma HLS PIPELINE
//	compute_loop:
//    for(int i = 0; i < N; i++){
//		#pragma HLS UNROLL complete
//        output[out_idx][i] = compute<IN_T>(input_buffers[i], kern_vals);
//    }
//    out_idx++;
//    if (out_idx*SCALE_D >= (in_idx + OFFSET - LANCZOS_A-1)*SCALE_N) step_input(input);
//}
//
//template <typename IN_T, typename OUT_T, int N, int IN_LEN, int OUT_LEN, int OFFSET>
//void Proc<IN_T, OUT_T, N, IN_LEN, OUT_LEN, OFFSET>::step_input(IN_T input[N][IN_LEN]){
//    for(int i = 0; i < N; i++){
//		#pragma HLS UNROLL complete
//        shift_up<IN_T, 2*LANCZOS_A>(input_buffers[i], in_idx >= IN_LEN? (IN_T) 0 : input[i][in_idx]);
//    }
//    in_idx++;
//}
//
//template <typename IN_T, typename OUT_T, int N, int IN_LEN, int OUT_LEN, int OFFSET>
//void Proc<IN_T, OUT_T, N, IN_LEN, OUT_LEN, OFFSET>::initialize(IN_T input[N][IN_LEN]){
//    // clear and initialize buffer with first few values of input
//    out_idx = 0;
//    int j = LANCZOS_A*2-1;
//    for (in_idx = -OFFSET; in_idx < LANCZOS_A*2 - OFFSET; in_idx++){
//        for(int i = 0; i < N; i++){
//            #pragma HLS UNROLL complete
//            input_buffers[i][j] =  in_idx < 0 ? (IN_T) 0 :input[i][in_idx];
//        }
//        j--;
//    }
//}

