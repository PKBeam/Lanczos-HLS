#include "lanczos.h"
#include "cyclic_buffer/cyclic_buffer.h"

// STREAM TODO
#include "hls_stream.h"


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


typedef CyclicBuffer<byte_t, int, LANCZOS_A*2, IN_HEIGHT + LANCZOS_A-1> cyclic_buffer_t;

class ColWorkers{
public:
	// offset = 0 when first out_idx is at the first pixel of the image.
	// want that in_idx - 1 < (out_idx-offset)/scale + LANCZOS_A <= in_idx  upon calculation
	const col_major_counter_t offset;
	col_major_counter_t curr_offset;
    // One buffer per input row, each buffer is23 2A in width
	//byte_t input_buffers[IN_WIDTH][LANCZOS_A*2];
	cyclic_buffer_t input_buffers[IN_WIDTH];

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

	col_major_counter_t in_idx = 0;
	col_major_counter_t out_idx = 0;

    ColWorkers(col_major_counter_t offset);
    void exec(stream_t in_img, kernel_t[2*LANCZOS_A], num_t[IN_WIDTH]);
    void step_input(stream_t input);
    void initialize(stream_t input);
    // out pos and write index are different: write index is the pointer offset. Out pos is the position of the
    // pixel being written to after the input image is upscaled by SCALE.
    void seek_write_index(col_major_counter_t idx);
    col_major_counter_t get_out_pos();

};

class RowWorkers{
public:
	// How many empty inputs there are upon initialize.
	// This translates to how shifted the image is after we perform the operation.
    const row_major_counter_t  offset;
    row_major_counter_t  curr_offset;

    // One buffer per input row, each buffer is 2A in width
	num_t input_buffers[LANCZOS_A*2];
//	CyclicBuffer<num_t, ap_uint<2>, LANCZOS_A*2> input_buffers;

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

	row_major_counter_t in_idx = 0;
	row_major_counter_t out_idx = 0;

    // Control logic to stop executing will be provided externally.
    RowWorkers(row_major_counter_t offset);
    void exec(num_t[IN_WIDTH], kernel_t[2*LANCZOS_A], byte_t[OUT_WIDTH]);
    void step_input(num_t[IN_WIDTH]);
    void initialize(num_t[IN_WIDTH]);
    // out pos and write index are different: write index is the pointer offset. Out pos is the position of the
    // pixel being written to after the input image is upscaled by SCALE.
    void seek_write_index(row_major_counter_t idx);
    row_major_counter_t get_out_pos();
};

#endif


