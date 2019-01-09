#define PROGRAM_FILE "ising.cl"
#define ISING_FUNC "ising_calc"
#define COUNT_FUNC "counter_incr"

#define sizeX 64
#define sizeY 64
#define iter 1000

#define prob_length 5

#define svec_length sizeX*sizeY

#ifdef __OPENCL_VERSION__
typedef char state_t;
typedef char4 state_v;
#else
typedef cl_char state_t;
typedef cl_char4 state_v;
#endif

#define max(x,y) ((x)>(y)?(x):(y))
#define ind(c,x,y) ( ((c)%iter)*svec_length + ((x)%sizeX)*sizeX + ((y)%sizeY) )
