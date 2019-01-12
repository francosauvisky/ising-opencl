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

// Calculate next cell state with metropolis algorithm. The cells are
// selected with a checkerboard pattern using a simple neighborhood,
// avoiding race conditions (updating two interacting cells simultaniously)
// > Call: 2D kernel with ranges [sizeX, sizeY]
kernel void
ising_calc(global state_t* states,
			  global uint* seeds,
			  global uint* iter_count,
			  global uint* probs)
{
	size_t i = get_global_id(0),
			 j = get_global_id(1);

	uint lcount = *iter_count,
		  lseed = seeds[lcount];

	state_t self_s = states[cind(lcount-1,i  ,j  )];
	state_t s_sum  = states[cind(lcount-1,i-1,j  )] +
						  states[cind(lcount-1,i+1,j  )] +
						  states[cind(lcount-1,i  ,j-1)] +
						  states[cind(lcount-1,i  ,j+1)];
			  s_sum *= self_s;

	uint par = ((i+j+(lcount%2))%2); // checkboard pattern (0 or 1)
	uint rand_sample = randomize_seed(lseed + 42*(sizeX*i + j));
	int flip = 1 - 2 * par * (rand_sample < probs[2+s_sum/2]);

	states[cind(lcount,i,j)] = self_s*flip;
}

// Add 1 to counter pointed by iter_count
// > Call: Task (1D kernel with range [1])
kernel void
counter_incr(global uint* iter_count)
{
	*iter_count += 1;
}

// Parallel sum for every cell value from a single state from iteration# *gind
// size of local buffer = local_size * sizeof(state_t)
// size of out buffer = iter * sizeof(uint) (if called for every state)
// > Call: 1D kernel with range [svec_length/2]
kernel void
ising_mag(global state_t* states,
			 global uint* gind,
			 global uint* out,
			 local state_t* state_buff)
{
	size_t i   = get_global_id(0),
			 i_l = get_local_id(0),
			 i_T = get_local_size(0),
			 gind_l = *gind;

	state_buff[i_L] = states[gind_l*svec_length + i]; // parallel copy to buffer

	barrier(CLK_LOCAL_MEM_FENCE);

	size_t delta = i_T;

	for(uint delta = i_T; delta > 0; delta >> 1)
	{
		state_buff[i_L] += state_buff[i_L + delta];
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	if(i_l == 0)
	{
		out[gind_l] += state_buff[0];
	}
}
