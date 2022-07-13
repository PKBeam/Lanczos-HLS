#include "lanczos.h"

#ifdef WORKER_TB
#include "worker_TB.h"
#endif

#ifdef FULL_TB
#include "full_TB.h"
#endif

int main(int argc, char* argv[]){
	sim_tb(argc, argv);
}
