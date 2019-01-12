#include "ising.h"
#include "ising-param.h"

#define NUM_QUEUE 4

// OpenCL global variables (can't be directly accessed outside this file)
cl_device_id device;
cl_context context;
cl_program program;
cl_command_queue queue[NUM_QUEUE];
int sys_count = 0;

#define num_plat 4
#define plat_id 1
/* Find a GPU or CPU associated with the first available platform 

The `platform` structure identifies the first platform identified by the 
OpenCL runtime. A platform identifies a vendor's installation, so a system 
may have an NVIDIA platform and an AMD platform. 

The `device` structure corresponds to the first accessible device 
associated with the platform. Because the second parameter is 
`CL_DEVICE_TYPE_GPU`, this device must be a GPU.
*/
cl_device_id
clh_create_device ()
{
	cl_platform_id platform[num_plat];
	cl_device_id dev;
	int err;

	// Identify a platform
	err = clGetPlatformIDs(num_plat, platform, NULL);
	if(err < 0) {
		perror("Couldn't identify a platform");
		exit(1);
	} 

	// Print platform name for debugging
	char infostr[100];
	clGetPlatformInfo(platform[plat_id],CL_PLATFORM_NAME,100,infostr,NULL);
	fprintf(stderr,"Platform name: %s\n",infostr);

	// Access a device
	// GPU
	err = clGetDeviceIDs(platform[plat_id], CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
	if(err == CL_DEVICE_NOT_FOUND) {
		// CPU
		fprintf(stderr,'USING CPU!\n');
		err = clGetDeviceIDs(platform[plat_id], CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
	}
	if(err < 0) {
		perror("Couldn't access any devices");
		exit(1);   
	}

	return dev;
}

// Create program from a file and compile it
cl_program
clh_build_program (cl_context ctx, cl_device_id dev, const char* filename)
{
	cl_program program;
	FILE *program_handle;
	char *program_buffer, *program_log;
	size_t program_size, log_size;
	int err;

	// Read program file and place content into buffer
	program_handle = fopen(filename, "r");
	if(program_handle == NULL) {
		perror("Couldn't find the program file");
		exit(1);
	}
	fseek(program_handle, 0, SEEK_END);
	program_size = ftell(program_handle);
	rewind(program_handle);
	program_buffer = (char*)malloc(program_size + 1);
	program_buffer[program_size] = '\0';
	fread(program_buffer, sizeof(char), program_size, program_handle);
	fclose(program_handle);

	/* Create program from file 

	Creates a program from the source code in the add_numbers.cl file. 
	Specifically, the code reads the file's content into a char array 
	called program_buffer, and then calls clCreateProgramWithSource.
	*/
	program = clCreateProgramWithSource(ctx, 1, 
		(const char**)&program_buffer, &program_size, &err);
	if(err < 0) {
		perror("Couldn't create the program");
		exit(1);
	}
	free(program_buffer);

	/* Build program 

	The fourth parameter accepts options that configure the compilation. 
	These are similar to the flags used by gcc. For example, you can 
	define a macro with the option -DMACRO=VALUE and turn off optimization 
	with -cl-opt-disable.
	*/
	err = clBuildProgram(program, 0, NULL, "-I.", NULL, NULL);
	if(err < 0) {
		/* Find size of log and print to std output */
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
				0, NULL, &log_size);
		program_log = (char*) malloc(log_size + 1);
		program_log[log_size] = '\0';
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG, 
				log_size + 1, program_log, NULL);
		printf("%s\n", program_log);
		free(program_log);
		exit(1);
	}

	return program;
}

// Initialize basic ising structures, build program and create kernels
int
ising_init()
{
	// Create device and context
	device = clh_create_device();
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if(err < 0) {
		perror("Couldn't create a context");
		return(1);   
	}

	// Create command queues 
	for (int i = 0; i < NUM_QUEUE; ++i)
	{
		queue[i] = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
	}
	if(err < 0) {
		perror("Couldn't create a command queue");
		return(1);   
	}

	// Build program
	program = clh_build_program(context, device, PROGRAM_FILE);


	return 0;
}

system_t
ising_new()
{
	system_t *newsys = malloc(sizeof *newsys);
	cl_int err = 0;

	// Create buffers
	newsys->state = clCreateBuffer(context, CL_MEM_READ_WRITE,
		iter*svec_length*sizeof(state_t), NULL, &err);
	newsys->rand_buff = clCreateBuffer(context, CL_MEM_READ_WRITE,
		iter*sizeof(cl_uint), NULL, &err);
	newsys->prob = clCreateBuffer(context, CL_MEM_READ_WRITE,
		prob_length*sizeof(cl_uint), NULL, &err);
	newsys->iter_count = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
		1*sizeof(cl_uint), &(cl_uint){1}, &err);
	if(err < 0) {
		perror("Couldn't create a buffer");
		exit(1);   
	}

	// Create kernels
	cl_uint k_num = 0;
	err = clCreateKernelsInProgram(program, NUM_KERNEL, newsys->kernel, &k_num);
	if((err < 0)||(k_num == 0)) {
		perror("Couldn't create a kernel");
		return(1);
	}

	// Set base kernel arguments
	err  = clSetKernelArg(newsys->kernel[0], 0, sizeof(cl_mem), &newsys->state);
	err |= clSetKernelArg(newsys->kernel[0], 1, sizeof(cl_mem), &newsys->rand_buff);
	err |= clSetKernelArg(newsys->kernel[0], 2, sizeof(cl_mem), &newsys->prob);
	err |= clSetKernelArg(newsys->kernel[0], 3, sizeof(cl_mem), &newsys->iter_count);
	if(err < 0) {
		perror("Couldn't create a kernel argument");
		exit(1);
	}

	sys_count++;
	return newsys;
}



int
ising_queue(system_t *cursys, uint iter, size_t op_num, ising_op *ops_todo)
{

}

int
ising_get(system_t *cursys, void *data)
{

}