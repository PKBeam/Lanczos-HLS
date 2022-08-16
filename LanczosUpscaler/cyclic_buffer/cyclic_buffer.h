#ifndef CYCLIC_BUFFER_H
#define CYCLIC_BUFFER_H

template <typename item_t, typename idx_t, int N, int M>
class CyclicBuffer {
public:
	static idx_t indices[N + 1];
	INLINE static idx_t next(idx_t x){
		return x < N? x + 1 : 0;
	}
	INLINE static idx_t prev(idx_t x){
		return x > 0 ? x - 1 : N;
	}
	CyclicBuffer(){
//		for (int i=0; i< M; i++){
//			slices[i] = new Slice(this);
//		}
#pragma HLS ARRAY_PARTITION variable=indices complete dim=1
#pragma HLS ARRAY_PARTITION variable=slices complete dim=2
		for (int i = 0; i < N + 1; i++){
			indices[i] = i;
		}
	};

//	~CyclicBuffer(){
//		for (int i=0; i< M; i++){
//			delete slices[i];
//		}
//	}
	INLINE void push(bool saturate) {
		for (int i = 0; i < N-1; i++){
#pragma HLS UNROLL
			indices[i] = indices[i + 1]; // shift indices down instead of data
		}

		// without saturate, the newest element becomes the write buffer, and the write buffer becomes the
		// last index shifted out.
		if (!saturate){
			indices[N-1] = indices[N];
		}
		indices[N] = prev(indices[0]);
	}

	// newest element appears at the top of the stack and is shifted down.
	INLINE void buffer_write(idx_t m, item_t val){
		slices[m].cyc_buf[indices[N]] = val;
	}

	class Slice {
	public:
		item_t cyc_buf[N+1];
		item_t operator [] (idx_t i) {
			return cyc_buf[indices[i]];
		}
	};


	Slice & operator [] (idx_t i) {
		return slices[i];
	}

private:
	Slice slices [M]; // dim1 is slots in the cycle. We use an extra slot as the write buffer.


};

template <typename item_t, typename idx_t, int N, int M>
idx_t CyclicBuffer<item_t, idx_t, N, M>::indices[N+1] = {0,1,2,3,4};

#endif

//#ifndef CYCLIC_BUFFER_H
//#define CYCLIC_BUFFER_H
//
//template <typename item_t, typename idx_t, int N, int MAX_LEN>
//class CyclicBuffer {
//public:
//
//	CyclicBuffer() {
//	#pragma HLS ARRAY_PARTITION variable=__cyc_buf factor=4 dim=1
//
//	}
//
//	// use these when N is an integer
//	void shift_left(item_t next) {
//		start_index++;
//		set(N - 1, next);
//	}
//
//	void shift_right(item_t next) {
//		start_index--;
//		set(0, next);
//	}
//
//	// use these when N = 2^M and idx_t is an M-bit unsigned integer
//	void shift_left_safe(item_t next) {
//		start_index++;
////		if (start_index >= N) {
////			(start_index = 0);
////		}
//		set(N - 1, next);
//	}
//
//	void shift_right_safe(item_t next) {
//		start_index--;
////		if (start_index < 0) {
////			(start_index = N-1);
////		}
//		set(0, next);
//	}
//
//	item_t get(idx_t i) {
//		return __cyc_buf[MIN(i + start_index, MAX_LEN-1) % N];
//	}
//
//	void set(idx_t i, item_t val) {
//		if (i + start_index < MAX_LEN-1){
//			__cyc_buf[(i + start_index) % N] = val;
//		}
//	}
//
//private:
//
//	item_t __cyc_buf[N];
//	idx_t start_index = 0;
//};
//
//#endif
