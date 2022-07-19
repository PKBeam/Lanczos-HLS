#include "lanczos.h"
typedef int input_idx_t;
typedef int output_idx_t;
typedef float scale_t;

kernel_t lanczos_kernel(input_idx_t, output_idx_t, scale_t);

kernel_t raw_lanczos_kernel(kernel_t x);
