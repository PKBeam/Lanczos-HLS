#include "kernel.h"
#include "hls_math.h"

#define PI ((kernel_t)M_PI)
#define PI_SQ ((kernel_t)(M_PI * M_PI))

kernel_t raw_lanczos_kernel(kernel_t x){
	if (x == 0){
		return (kernel_t) 1.0;
	} else {
		return (kernel_t) (LANCZOS_A*hls::sinpi(x)*hls::sinpi(x / LANCZOS_A) / (PI_SQ*x*x));
	}
}

kernel_t raw_lanczos_kernel(input_idx_t input_idx, output_idx_t output_idx, scale_t scale){
	kernel_t in_arg = (kernel_t)(output_idx/scale - input_idx);
	return raw_lanczos_kernel(in_arg);
}
