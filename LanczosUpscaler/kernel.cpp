#include "hls_math.h"
#include "kernel.h"


#define PI ((num_t)M_PI)
#define PI_SQ ((num_t)(M_PI * M_PI))

#define B ((kernel_t)0.160102449173)
#define K1 ((kernel_t)0.0605)
#define K3 ((kernel_t)0.247479224531)

//kernel_t raw_lanczos_kernel(kernel_t x){
//	kernel_t s = x*x;
//	kernel_t s1 = (kernel_t)1.0-s;
//	kernel_t s4 = (kernel_t)1.0 - s*(kernel_t)0.25;
//	kernel_t sk1 = (kernel_t)1.0 - K1*s;
//	kernel_t sk3 = (kernel_t)1.0 - K3*s;
//	kernel_t p1 = s4*sk1;
//	kernel_t p2 = s4*(kernel_t)(s1*sk3);
//	kernel_t ps1 = p1*p1;
//
//	return s4*s4*s1*(sk1*sk1 - s*s1*sk3*sk3*B);
////	return ps1 - s*(ps1 + p2*p2*B);
//}

kernel_t sinc(kernel_t x) {
    return sin(x)/x;
}

kernel_t lanczos_kernel(kernel_t x) {
    return sinc(M_PI * x) * sinc(M_PI * x / LANCZOS_A);
}

kernel_t lanczos_kernel(input_idx_t input_idx, output_idx_t output_idx, scale_t scale){
	kernel_t in_arg = (kernel_t)(output_idx*((kernel_t)1.0/SCALE) - input_idx*SCALE_N);
	return raw_lanczos_kernel(in_arg);
}
