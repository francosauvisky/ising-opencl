#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>
#include "opencl-helper.c"
#define PROGRAM_FILE "ising.cl"
#define KERNEL_FUNC "ising"

#define sizeX 64
#define sizeY 64

typedef cl_char state_t
#define svec_length sizeX*sizeY

int
main ()
{
   // OpenCL structures
   cl_device_id device;
   cl_context context;
   cl_program program;
   cl_kernel kernel[2];
   cl_command_queue queue;
   cl_mem in_buffer, out_buffer;
   cl_int i, j, err;
   size_t local_size = 256, global_size = sizeX*sizeY;

   // Create device and context
   device = clh_create_device();
   context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
   if(err < 0) {
      perror("Couldn't create a context");
      exit(1);   
   }

   // Build program
   program = clh_build_program(context, device, PROGRAM_FILE);

   // Create a command queue 
   queue = clCreateCommandQueue(context, device, 0, &err);
   if(err < 0) {
      perror("Couldn't create a command queue");
      exit(1);   
   };

   // Create kernels
   kernel[0] = clCreateKernel(program, KERNEL_FUNC, &err);
   kernel[1] = clCreateKernel(program, KERNEL_FUNC, &err);
   if(err < 0) {
      perror("Couldn't create a kernel");
      exit(1);
   };

   // Initialize data with random bits
   state_t data[svec_length];
   for(int i = 0; i < svec_length; i++)
   {
      data[i] = 2*(rand() < (RAND_MAX/2))-1;
   }

   // Initialize uniform probability distribution
   float prob_vec[NUM_NEIGHBORS];
   for (int i = 0; i < NUM_NEIGHBORS; i++)
   {
      prob_vec = 1.0/NUM_NEIGHBORS;
   }

   // Create input/output data buffer (and copy data to input)
   in_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY |
         CL_MEM_COPY_HOST_PTR, svec_length * sizeof(state_t), data, &err);
   out_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
         svec_length * sizeof(state_t), NULL, &err);
   if(err < 0) {
      perror("Couldn't create a buffer");
      exit(1);   
   };

   // Set kernel 1 arguments
   err = clSetKernelArg(kernel[0], 0, sizeof(cl_mem), &in_buffer);
   err |= clSetKernelArg(kernel[0], 1, sizeof(cl_mem), &out_buffer);
   err |= clSetKernelArg(kernel[0], 2, NUM_NEIGHBORS*sizeof(float), &prob_vec);
   err |= clSetKernelArg(kernel[0], 3, sizeof(cl_uint), sizeX);
   if(err < 0) {
      perror("Couldn't create a kernel argument");
      exit(1);
   }

   // Set kernel 2 arguments (backwards)
   err = clSetKernelArg(kernel[1], 0, sizeof(cl_mem), &out_buffer);
   err |= clSetKernelArg(kernel[1], 1, sizeof(cl_mem), &in_buffer);
   err |= clSetKernelArg(kernel[1], 2, NUM_NEIGHBORS*sizeof(float), &prob_vec);
   err |= clSetKernelArg(kernel[1], 3, sizeof(cl_uint), sizeX);
   if(err < 0) {
      perror("Couldn't create a kernel argument");
      exit(1);
   }

   // Enqueue kernel
   err = clEnqueueNDRangeKernel(queue, kernel[0], 1, NULL, &global_size, 
         &local_size, 0, NULL, NULL); 
   if(err < 0) {
      perror("Couldn't enqueue the kernel");
      exit(1);
   }

   // Read the kernel's output
   err = clEnqueueReadBuffer(queue, out_buffer, CL_TRUE, 0, 
          global_size * sizeof(int), data, 0, NULL, NULL);
   if(err < 0) {
      perror("Couldn't read the buffer");
      exit(1);
   }

   // Print results
   for(int i = 0; i < sizeX; i++)
   {
      for(int j = 0; j < sizeY; j++)
      {
         printf('%c',(data[sizeX*i+j] == 1)? 1:0);
      }
      printf('\n');
   }

   // Deallocate resources
   clReleaseKernel(kernel);
   clReleaseMemObject(out_buffer);
   clReleaseMemObject(in_buffer);
   clReleaseCommandQueue(queue);
   clReleaseProgram(program);
   clReleaseContext(context);
   return 0;
}
