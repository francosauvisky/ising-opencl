#include "ising_param.h"

typedef int state_t;
typedef int4 state_v;

// Based on "4-byte Integer Hashing" by Thomas Wang
// http://www.burtleburtle.net/bob/hash/integer.html
uint randomize_seed(uint a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}

#define ind(c,x,y) ((c)*svec_length)+(sizeX*((x)%sizeX)+((y)%sizeY))
kernel void ising(global state_t* states, global uint* seeds,
   global uint* count, global uint* probs)
{
   size_t i = get_global_id(0);
   size_t j = get_global_id(1);
   uint lcount = *count;
   uint lseed = seeds[lcount];

   uint par = ((i+j+(lcount%2))%2); // checkboard pattern

   if(i+j == 0)
   {
      *count = lcount + 1;
   }

   barrier(CLK_LOCAL_MEM_FENCE);

   state_t self_s  =  states[ind(lcount-1,i  ,j  )];
   state_t neigh_1 =  states[ind(lcount-1,i-1,j  )];
   state_t neigh_2 =  states[ind(lcount-1,i+1,j  )];
   state_t neigh_3 =  states[ind(lcount-1,i  ,j-1)];
   state_t neigh_4 =  states[ind(lcount-1,i  ,j+1)];

   barrier(CLK_LOCAL_MEM_FENCE);

   uint rand_sample = randomize_seed(lseed + 42*(sizeX*i + j));

   int s_sum = self_s * (neigh_1 + neigh_2 + neigh_3 + neigh_4);

   int flip = 1-2*par*(rand_sample < probs[2+s_sum/2]);

   states[ind(lcount,i,j)] = self_s*flip;
}
