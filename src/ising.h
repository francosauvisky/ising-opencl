#ifndef ISING_HEADER
#define ISING_HEADER

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>

#define NUM_KERNEL 5

typedef struct ising_ocl
{
	cl_kernel kernel[NUM_KERNEL];
	cl_mem state;
	cl_mem rand_buff;
	cl_mem iter_count;
	cl_mem prob;
	cl_mem output;
} system_t;

int ising_init(void);
system_t ising_new();
int ising_free(system_t *);
int ising_run(system_t *);

#endif

/*
call ising_init() on start for OpenCL configuration & kernel building

allocate memory buffers on device with ising_new()

configure system: initial state, probabilities, random buffer, measurements

run system (fill buffer) with ising_run()

get results with ising_read()

free buffers with ising_free()
*/
