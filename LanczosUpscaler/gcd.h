// gcd.h
// INPUT: #define SIMP_INPUT_A
// INPUT: #define SIMP_INPUT_B

#ifndef SIMP_COUNTER
	#define SIMP_COUNTER 0
	#define SIMP_A_FACTOR 1
	#define SIMP_B_FACTOR 1
	#define SIMP_A_TEMP SIMP_INPUT_A*SIMP_A_FACTOR
	#define SIMP_B_TEMP SIMP_INPUT_B*SIMP_B_FACTOR
#endif

#define SIMP_A_POS_DIFF (SIMP_A_TEMP-1)/SIMP_INPUT_B
#define SIMP_B_POS_DIFF (SIMP_B_TEMP-1)/SIMP_INPUT_A
#if (SIMP_A_TEMP < SIMP_B_TEMP)
	#include "util_includes/simp/INC_SIMP_A.h"
	#include "gcd.h"
#elif (SIMP_A_TEMP > SIMP_B_TEMP)
	#include "util_includes/simp/INC_SIMP_B.h"
	#include "gcd.h"
#endif

#define SIMP_OUTPUT_A SIMP_B_FACTOR
#define SIMP_OUTPUT_B SIMP_A_FACTOR
