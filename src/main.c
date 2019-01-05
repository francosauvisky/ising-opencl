#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CL/cl.h>

#include "ising-param.h"
#include "opencl-helper.c"

typedef cl_int state_t;

int
main ()
{
   // OpenCL structures
   cl_device_id device;
   cl_context context;
   cl_program program;
   cl_kernel kernel_ising;
   cl_command_queue queue;
   cl_mem prob_buffer, rand_buffer, data_buffer, count_buffer;
   cl_event calc_done[iter];
   cl_int i, j, err;
   size_t local_size[2] = {16,16}, global_size[2] = {sizeX,sizeY};

   // Create device and context
   device = clh_create_device();
   context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
   if(err < 0) {
      perror("Couldn't create a context");
      exit(1);   
   }

   // Probability vector (index = number of aligned spins in neighborhood?)
   cl_uint prob[prob_length];
   for (int i = 0; i < prob_length; i++)
   {
      prob[i] = ( (i < 2)? 1 : exp(-2.0*(i-2)) ) * (float)CL_UINT_MAX;
   }

   // Initial data with random bits (hot start)
   state_t initial_sys[svec_length];
   for(int i = 0; i < svec_length; i++)
   {
      initial_sys[i] = (rand() < (RAND_MAX/2))?1:-1;
   }

   // Random seeds (1 per iteration)
   cl_uint rand_seed[iter];
   for(int i = 0; i < iter; i++)
   {
      rand_seed[i] = rand();
   }

   // Allocate memory for the simulation (after completion)
   state_t *f_sys = malloc(iter*svec_length*sizeof(state_t));
   for (int i = 0; i < iter*svec_length; i++)
   {
      f_sys[i] = 0;
   }

   // Create data buffers
   data_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
      iter*svec_length*sizeof(state_t), f_sys, &err);
   rand_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
      iter*sizeof(cl_uint), rand_seed, &err);
   prob_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
      prob_length*sizeof(cl_uint), prob, &err);
   count_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
      1*sizeof(cl_uint), &(cl_uint){1}, &err);
   if(err < 0) {
      perror("Couldn't create a buffer");
      exit(1);   
   };

   // Create command queues 
   queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
   if(err < 0) {
      perror("Couldn't create a command queue");
      exit(1);   
   };

   // Build program
   program = clh_build_program(context, device, PROGRAM_FILE);
  
   // Create kernels
   kernel_ising = clCreateKernel(program, ISING_FUNC, &err);
   if(err < 0) {
      perror("Couldn't create a kernel");
      exit(1);
   };

   // Set kernels arguments
   err  = clSetKernelArg(kernel_ising, 0, sizeof(cl_mem), &data_buffer);
   err |= clSetKernelArg(kernel_ising, 1, sizeof(cl_mem), &rand_buffer);
   err |= clSetKernelArg(kernel_ising, 2, sizeof(cl_mem), &count_buffer);
   err |= clSetKernelArg(kernel_ising, 3, sizeof(cl_mem), &prob_buffer);

   if(err < 0) {
      perror("Couldn't create a kernel argument");
      exit(1);
   }

   err = clEnqueueWriteBuffer(queue,data_buffer,CL_TRUE,0,svec_length*sizeof(state_t),
      initial_sys,0,NULL,NULL);
   err |= clEnqueueMarker(queue,&calc_done[0]);

   // Enqueue kernels
   for(int i = 1; i < iter; i++)
   {
      fprintf(stderr,"Enqueue loop %d\n", i);
      err |= clEnqueueNDRangeKernel(queue, kernel_ising, 2, NULL, global_size, 
         local_size, 1, &calc_done[i-1], &calc_done[i]);

      if(err < 0){
         perror("Couldn't enqueue the kernel");
         exit(1);
      }

      // Limits size of queue
      // if((i%10)==0)
      // {
      //    clWaitForEvents(1, &calc_done[i]);
      // }
   }
   clFlush(queue);
   clFinish(queue);

   // Read the kernel's output
   err = clEnqueueReadBuffer(queue, data_buffer, CL_TRUE, 0,
      iter*svec_length*sizeof(state_t), f_sys, 0, NULL, NULL);
   if(err < 0) {
      printf("%d\n",err);
      perror("Couldn't read the buffer");
      exit(1);
   }

   double nanoSeconds = 0;
   for (int i = 0; i < iter; ++i)
   {
      cl_ulong time_start;
      cl_ulong time_end;

      clGetEventProfilingInfo(calc_done[i], CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
      clGetEventProfilingInfo(calc_done[i], CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
      nanoSeconds += time_end-time_start;
   }

   printf("OpenCl Execution time is: %0.3f milliseconds \n",nanoSeconds / 1e6);

   // Print data collected
   for(int k = 0; k < iter; k++)
   {
      for(int i = 0; i < sizeX; i++)
      {
         for (int j = 0; j < sizeY; ++j)
         {
            // printf("%d ",(f_sys[svec_length*k+sizeX*i+j]));
            printf("%c",(f_sys[svec_length*k+sizeX*i+j]==1?'+':'-'));
         }
         printf("\n");
      }
      printf("\n");
   }

   // Deallocate resources (IMPORTANT!)
   clReleaseKernel(kernel_ising);
   clReleaseMemObject(rand_buffer);
   clReleaseMemObject(prob_buffer);
   clReleaseMemObject(count_buffer);
   clReleaseMemObject(data_buffer);
   clReleaseCommandQueue(queue);
   clReleaseProgram(program);
   clReleaseContext(context);
   free(f_sys);
   return 0;
}
