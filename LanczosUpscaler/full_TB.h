//
//  main.c
//  LanczosUpscaler
//
//  Created by Vincent Liu on 18/6/2022.
//
#include "lanczos.h"
#include <math.h>

// STREAM TODO
#include "hls_stream.h"


#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"

#define DEBUG 1
typedef ap_uint<8> byte;

byte img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH];
byte img_out_ex[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH];

byte_t  img_in_ob[IN_HEIGHT][IN_WIDTH];
byte_t  img_out_ob[OUT_HEIGHT][OUT_WIDTH];

rgb_pixel_t img_interlaced_out_ex[OUT_HEIGHT * OUT_WIDTH];
rgb_pixel_t img_interlaced_out_ob[OUT_HEIGHT * OUT_WIDTH];

byte double_to_uint8(double x) {
    if (x > UINT8_MAX) {
        return UINT8_MAX;
    } else if (x < 0){
        return 0;
    } else {
    	return (byte) x;
    }
}

double sinc(double x) {
	if (x==0){
		return 1;
	}
    return sin(x)/x;
}

#define _S1(a) #a
#define _S(a) _S1(a)

#define TEST_ID _S(IN_WIDTH) "x" _S(IN_HEIGHT) "-" _S(OUT_WIDTH) "x" _S(OUT_HEIGHT) "-" _S(SCALE_N) "|" _S(SCALE_D) "-"

double lanczos_kernel(double x) {
    return sinc(M_PI * x) * sinc(M_PI * x / LANCZOS_A);
}

void lanczos_interpolate_row(byte in[IN_WIDTH], byte out[OUT_WIDTH]) {
    for (int xx = 0; xx < OUT_WIDTH; xx++) {
        double x = (double) xx / SCALE;
        double sum = 0;
        for (int i = MAX(0, floor(x) - LANCZOS_A + 1); i <= MIN(IN_WIDTH - 1, floor(x) + LANCZOS_A); i++) {
            sum += in[i] * lanczos_kernel(x - i);
        }

        out[xx] = double_to_uint8(sum);
    }
}

void lanczos_interpolate_col(byte img[OUT_HEIGHT][OUT_WIDTH], int col) {
    // start filling from largest height first so we don't overwrite any pixels we're using
    for (int xx = OUT_HEIGHT - 1; xx >= 0; xx--) {
        double x = (double) xx / SCALE;
        double sum = 0;
        for (int i = MAX(0, floor(x) - LANCZOS_A + 1); i <= MIN(IN_HEIGHT - 1, floor(x) + LANCZOS_A); i++) {
            sum += img[i][col] * lanczos_kernel(x - i);
        }
        img[xx][col] = double_to_uint8(sum);
    }
}

void lanczos_expected(
    byte img_in[NUM_CHANNELS][IN_HEIGHT][IN_WIDTH],
    byte img_out[NUM_CHANNELS][OUT_HEIGHT][OUT_WIDTH]
) {
    for (int i = 0; i < IN_HEIGHT; i++) {
        for (int j = 0; j < NUM_CHANNELS; j++) {
            lanczos_interpolate_row(img_in[j][i], img_out[j][i]);
        }
    }

    for (int i1 = 0; i1 < OUT_WIDTH; i1++) {
        for (int j = 0; j < NUM_CHANNELS; j++) {
            lanczos_interpolate_col(img_out[j], i1);
        }
    }

    return;
}


int sim_tb(int argc, char* argv[]) {
    int width;
    int height;
    int channels;

    // STREAM TODO
    hls::stream<byte_t> stream_in;
    hls::stream<byte_t> stream_out;
    rgb_pixel_t* img = (rgb_pixel_t*) stbi_load(OUT_DIR IN_IMG, &width, &height, &channels, NUM_CHANNELS);

    // error checking
    if (img == NULL) {
        printf("Image was not loaded successfully.\n");
        return EXIT_FAILURE;
    }

    if (width != IN_WIDTH || height != IN_HEIGHT) {
        printf("Image has wrong dimensions (%i x %i).\n", width, height);
        return EXIT_FAILURE;
    }

    if (channels != NUM_CHANNELS) {
        printf("Image has wrong amount of channels (expected %i, got %i).\n", NUM_CHANNELS, channels);
        return EXIT_FAILURE;
    }
    printf("Scale:%d/%d, WIDTHS %d -> %d\n",SCALE_N, SCALE_D, IN_WIDTH, OUT_WIDTH);

    // read image data
    for (int i = 0; i < IN_WIDTH * IN_HEIGHT; i++) {
        int row = i / IN_WIDTH;
        int col = i % IN_WIDTH;
        byte_t pixel;
        for (int j = 0; j < NUM_CHANNELS; j++) {
            img_in[j][row][col] = img[i].channel[j];
//            img_in_ob[row][col].write(j, img[i].channel[j]);
            // STREAM TODO
            pixel.ch[j]= img[i].channel[j];
        }
        stream_in.write(pixel);
    }

    lanczos(stream_in, stream_out);
    lanczos_expected(img_in, img_out_ex);

    double err = 0;
    // copy image data back
    byte_t r_pixel;
    for (int i = 0; i < OUT_WIDTH * OUT_HEIGHT; i++) {
        int row = i / OUT_WIDTH;
        int col = i % OUT_WIDTH;

        rgb_pixel_t pixel_ex = {};
        rgb_pixel_t pixel_ob = {};
        // STREAM TODO
        stream_out.read(r_pixel);
        for (int j = 0; j < NUM_CHANNELS; j++) {

        	pixel_ex.channel[j] = img_out_ex[j][row][col];
        	pixel_ob.channel[j] = r_pixel.ch[j];
        	// STREAM TODO
        	//stream_out.read(pixel_ob.channel[j];
            int diff = (int) pixel_ex.channel[j]- (int) pixel_ob.channel[j];
            err += diff*diff;
        }
        img_interlaced_out_ex[i] = pixel_ex;
        img_interlaced_out_ob[i] = pixel_ob;
    }
    printf("RMS err: %.3f\n", sqrt((double)err/(NUM_CHANNELS*OUT_WIDTH*OUT_HEIGHT)));
    char str[100];


    sprintf(str, OUT_DIR "%dx%d->%dx%d_%d|%d_%d-" OUT_IMG_EX, IN_WIDTH,IN_HEIGHT,OUT_WIDTH,OUT_HEIGHT,SCALE_N, SCALE_D, LANCZOS_A);
    // save data to both observed and expected
    stbi_write_png(str, OUT_WIDTH, OUT_HEIGHT, NUM_CHANNELS, img_interlaced_out_ex, OUT_WIDTH * NUM_CHANNELS);


    sprintf(str, OUT_DIR "%dx%d->%dx%d_%d|%d_%d-" OUT_IMG_OB, IN_WIDTH,IN_HEIGHT,OUT_WIDTH,OUT_HEIGHT,SCALE_N, SCALE_D,LANCZOS_A);

    stbi_write_png(str, OUT_WIDTH, OUT_HEIGHT, NUM_CHANNELS, img_interlaced_out_ob, OUT_WIDTH * NUM_CHANNELS);

    return 0;
}
