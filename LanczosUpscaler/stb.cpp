
#define STBI_NO_SIMD
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"


int gcd(int a, int b){
	if (b != 0) return gcd(b, a%b);
	return a;
}

