#include "lanczos.h"
#include "worker.h"

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
    for (int k = N-2; k >= 0; k--) reg[k] = reg[k-1];
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
    void exec(byte_t input[N][IN_LEN], kernel_t kern_vals[2*LANCZOS_A], num_t output[OUT_LEN][IN_LEN]){
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
