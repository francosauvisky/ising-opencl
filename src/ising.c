#include "ising.h"
#include "ising-param.h"
#include "opencl-helper.c"

#define NUM_QUEUE 2
#define NUM_KERNEL 5

// OpenCL global variables (private to this file)
cl_device_id device;
cl_context context;
cl_program program;
cl_command_queue queue[NUM_QUEUE];
cl_kernel kernel[NUM_KERNEL];

// OpenCL simulation "macro"-structure:
typedef struct ising_ocl
{
	cl_mem state;
	cl_mem rand_buff;
	cl_mem iter_count;
	cl_mem prob;
} system_t;

typedef enum ising_operations
{
	get_state = 0x01;
	get_magnet = 0x02;
	get_energy = 0x04;
} ising_op;

int
ising_init()
{
	// Create device and context
	device = clh_create_device();
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if(err < 0) {
		perror("Couldn't create a context");
		exit(1);   
	}

	// Create command queues 
	for (int i = 0; i < NUM_QUEUE; ++i)
	{
		queue[i] = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
	}
	if(err < 0) {
		perror("Couldn't create a command queue");
		exit(1);   
	}


	// Build program
	program = clh_build_program(context, device, PROGRAM_FILE);

	// Create kernels
	kernel_ising = clCreateKernel(program, ISING_FUNC, &err);
	if(err < 0) {
		perror("Couldn't create a kernel");
		exit(1);
	};

}

int
ising_queue(system_t *cursys, uint iter, size_t op_num, ising_op *ops_todo)
{

}

int
ising_get(system_t *cursys, void *data)
