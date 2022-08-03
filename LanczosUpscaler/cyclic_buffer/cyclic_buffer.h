#ifndef CYCLIC_BUFFER_H
#define CYCLIC_BUFFER_H


template <typename item_t, typename idx_t, int N, int MAX_LEN>
class CyclicBuffer {
public:

	CyclicBuffer() {
#pragma HLS ARRAY_PARTITION variable=__cyc_buf factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=__cyc_buf complete dim=2

	}

	// use these when N is an integer
	void shift_left(item_t next) {
		start_index++;
		set(N - 1, next);
	}

	void shift_right(item_t next) {
		start_index--;
		set(0, next);
	}

	// use these when N = 2^M and idx_t is an M-bit unsigned integer
	void shift_left_safe(item_t next) {
		start_index++;
//		if (start_index >= N) {
//			(start_index = 0);
//		}
		set(N - 1, next);
	}

	void shift_right_safe(item_t next) {
		start_index--;
//		if (start_index < 0) {
//			(start_index = N-1);
//		}
		set(0, next);
	}

	INLINE item_t get(idx_t i) {
		return __cyc_buf[MIN(i + start_index, MAX_LEN-1) % N];
	}

	INLINE item_t operator [] (idx_t i) {

		return __cyc_buf[MIN(i + start_index, MAX_LEN-1) % N];
	}
	
	INLINE void set(idx_t i, item_t val) {
		if (i + start_index < MAX_LEN-1){
			__cyc_buf[(i + start_index) % N] = val;
		}
	}

private:

	item_t __cyc_buf[N];
	idx_t start_index = 0;
};

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
