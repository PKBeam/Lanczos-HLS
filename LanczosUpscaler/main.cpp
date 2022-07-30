#include "lanczos.h"

#define FULL_TB

#ifdef FULL_TB
#define TB_NAME "full TB"
#include "full_TB.h"
#endif
#ifdef WORKER_TB
#define TB_NAME "worker TB"
#include "worker_TB.h"
#include "kernel.h"
#endif

int main(int argc, char* argv[]){
	printf("Running " TB_NAME "\n");
	printf ("%d\n", SCALE_D, SCALE_N);
	sim_tb(argc, argv);
}
