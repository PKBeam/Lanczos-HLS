#include "kernel.h"
#include "hls_math.h"

//#include "math.h"


//#define B ((kernel_t)0.160102449173)
//#define K1 ((kernel_t)0.0605)
//#define K3 ((kernel_t)0.247479224531)


kernel_t raw_lanczos_kernel(kernel_t x) {
	if (x == 0){
		return 1;
	} else {
		return kernel_t(LANCZOS_A/(M_PI*M_PI)) * hls::sinpi(x)*hls::sinpi(x/LANCZOS_A)/(x*x);
	}
}

//
//
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





void init_lanczos_kernel(kernel_t ROM[LANCZOS_A*SCALE_N+1] ) {
	for (int i = 0; i < LANCZOS_A*SCALE_N; i++){
		ROM[i] = (kernel_t)raw_lanczos_kernel((kernel_t)i/SCALE_N);
	}
	ROM[LANCZOS_A*SCALE_N] = 0;
}



// LUT for the lanczos kernel for a = 2
kernel_t raw_lanczos_kernel_LUT_a2(input_idx_t input_idx, output_idx_t output_idx) {
	#pragma HLS INLINE

	kernel_t lanczos_LUT_a2[LANCZOS_A*SCALE_N+1];
	init_lanczos_kernel(lanczos_LUT_a2);
	#pragma HLS ARRAY_PARTITION variable=lanczos_LUT_a2 complete dim=1
	output_idx_t x = abs_<output_idx_t>(output_idx*SCALE_D - input_idx*SCALE_N);

	return lanczos_LUT_a2[x];
}

kernel_t lanczos_kernel(input_idx_t input_idx, output_idx_t output_idx, scale_t scale){
	//kernel_t in_arg = (kernel_t)(output_idx*((kernel_t)1.0/SCALE) - input_idx);
	//return raw_lanczos_kernel(in_arg);
	return raw_lanczos_kernel_LUT_a2(input_idx, output_idx);

//	return raw_lanczos_kernel(fractional_t(output_idx/scale) - input_idx);
}
