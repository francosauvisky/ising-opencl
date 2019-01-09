#include "ising-param.h"

// Based on "4-byte Integer Hashing" by Thomas Wang
// http://www.burtleburtle.net/bob/hash/integer.html
inline uint
randomize_seed(uint a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}
   
kernel void
ising_calc(global state_t* states,
           global uint* seeds,
           global uint* count,
           global uint* probs)
{
   size_t i = get_global_id(0), j = get_global_id(1);
   uint lcount = *count, lseed = seeds[lcount];

   uint par = ((i+j+(lcount%2))%2); // checkboard pattern (0 or 1)

   state_t self_s = states[ind(lcount-1,i  ,j  )];
   state_t s_sum  = states[ind(lcount-1,i-1,j  )] +
                    states[ind(lcount-1,i+1,j  )] +
                    states[ind(lcount-1,i  ,j-1)] +
                    states[ind(lcount-1,i  ,j+1)];
           s_sum *= self_s;

   uint rand_sample = randomize_seed(lseed + 42*(sizeX*i + j));

   int flip = 1-2*par*(rand_sample < probs[2+s_sum/2]);

   states[ind(lcount,i,j)] = self_s*flip;
}

kernel void
counter_incr(global uint* count)
{
   *count += 1;
}
