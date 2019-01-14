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

//  MWC64X Random Number Generator
uint MWC64X(uint2 *state)
{
    enum {A=4294883355U};
    uint x=(*state).x, c=(*state).y;  // Unpack the state
    uint res=x^c;                     // Calculate the result
    uint hi=mul_hi(x,A);              // Step the RNG
    x=x*A+c;
    c=hi+(x<c);
    *state=(uint2)(x,c);              // Pack the state back up
    return res;                       // Return the next result
}

// Calculate next cell state with Metropolis algorithm. The cells are
// selected with a checkerboard pattern using a simple neighborhood,
// avoiding race conditions (updating two interacting cells simultaniously)
// > Call: 2D kernel with ranges [sizeX, sizeY]
kernel void
ising_calc(global state_t* states,
		   global uint* seeds,
		   global uint* iter_count,
		   global uint* probs,
		   global uint* prob_n)
{
	size_t i = get_global_id(0),
		   j = get_global_id(1);
	uint lcount = *iter_count,
		  lseed = seeds[lcount],
		  lprob = *prob_n;

	state_t self_s = states[cfind(lcount-1,i  ,j  )];
	state_t s_sum  = states[cind(lcount-1,i-1,j  )] +
					 states[cind(lcount-1,i+1,j  )] +
					 states[cind(lcount-1,i  ,j-1)] +
					 states[cind(lcount-1,i  ,j+1)];
			  s_sum *= self_s;

	uint par = ((i+j+(lcount%2))%2); // checkboard pattern (0 or 1)
	uint rand_sample = randomize_seed(randomize_seed(lseed + 42013*(sizeX*i + j)));
	int flip = 1 - 2 * par *
	(rand_sample < probs[prob_length*lprob + prob_zero + s_sum/2]);

	states[cfind(lcount,i,j)] = self_s*flip;
}

// Parallel sum every cell value from a single state from iteration # *gind
// size of local buffer = local_size * sizeof(int)
// size of out buffer = iter * sizeof(uint) (if called for every state)
// > Call: 1D kernel with range [svec_length/2]
kernel void
ising_mag(global state_t* states,
		  global uint* gind,
		  global int* out,
		  local int* state_buff)
{
	size_t i   = get_global_id(0),
		   i_l = get_local_id(0),
		   i_T = get_local_size(0),
		   gind_l = *gind;

	state_buff[i_l] = states[gind_l*svec_length + i]; // parallel copy to local buffer

	barrier(CLK_LOCAL_MEM_FENCE);

	// Sum local buffer
	for(int delta = i_T; delta > 0; delta >>= 1)
	{
		state_buff[i_l] += (delta>=i_l)*state_buff[i_l + delta];
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	if(i_l == 0)
	{
		atomic_add(&out[gind_l], state_buff[0]);
	}
}

// Reshuffle random buffer (PRNG)
// > Call: 1D kernel with range [iter]
kernel void
ising_rand(global uint *seeds)
{
	size_t i = get_global_id(0);

	seeds[i] = randomize_seed(randomize_seed(randomize_seed(seeds[i])));
}

kernel void
next_prob(global uint* prob_count)
{
	*prob_count += 1;
}

kernel void
counter_incr(global uint* counter)
{
	*counter += 1;
}
