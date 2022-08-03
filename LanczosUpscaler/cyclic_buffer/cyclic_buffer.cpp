#include "cyclic_buffer.h"

template <typename item_t, typename idx_t, int N>
item_t CyclicBuffer::get(idx_t i) {
	return __buffer[(i + start_index) % N];
}

template <typename item_t, typename idx_t, int N>
void CyclicBuffer::set(idx_t i, item_t val) {
	__buffer[(i + start_index) % N] = val;
}

template <typename item_t, typename idx_t, int N>
void CyclicBuffer::shift_left(item_t next) {
	start_index++;
	if (start_index >= N) {
		(start_index = 0);
	}
	set(-1, next);
}

template <typename item_t, typename idx_t, int N>
void CyclicBuffer::shift_right(item_t next) {
	start_index--;
	if (start_index < 0) {
		(start_index = N-1);
	}
	set(0, next);
}

template <typename item_t, typename idx_t, int N>
void CyclicBuffer::shift_left_overflow(item_t next) {
	start_index++;
	set(-1, next);
}

template <typename item_t, typename idx_t, int N>
void CyclicBuffer::shift_right_overflow(item_t next) {
	start_index--;
	set(0, next);
}
