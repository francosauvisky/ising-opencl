#include "ising.h"

#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>
#include <sys/ioctl.h>

int _kbhit();

int
main ()
{
	int *mag_data = malloc(iter*sizeof(uint));
	state_t *states_data = malloc(iter*svec_length*sizeof(state_t));
	state_t *initial = malloc(svec_length*sizeof(state_t));

	srand((uint)time(NULL));
	for (int i = 0; i < svec_length; ++i)
	{
		// initial[i] = (rand() < (RAND_MAX/2))? 1 : -1;
		initial[i] = 1-2*((i%sizeX)<(i/sizeX));
		if (((i%sizeX) == (i/sizeX)) & (abs(i/sizeX - sizeX/2) > 4)) initial[i] = 0;
		if (((i%sizeX) == (i/sizeX)+1) & (abs(i/sizeX - sizeX/2) > 4)) initial[i] = 0;
		if((i%sizeX)==0) initial[i]=0;
		if((i/sizeX)==0) initial[i]=0;
		if((i%sizeX)==(sizeX-1)) initial[i]=0;
		if((i/sizeX)==(sizeX-1)) initial[i]=0;
	}

	initial[svec_length/2+sizeX/2] = 10;
	initial[svec_length/2-1+sizeX/2] = -10;
	initial[svec_length/2+1+sizeX/2] = 10;
	initial[svec_length/2+2+sizeX/2] = -10;

	// annealing
	cl_float betas[prob_buff];
	for (int i = 0; i <= prob_buff; ++i)
	{
		betas[i] = 1 + 2*pow(i/(cl_float)prob_buff,2);
		// printf("%f\n", betas[i]);
	}

	ising_init();
	system_t mysys = ising_new();
	ising_configure(&mysys, initial, 0.0);
	ising_configure_betas(&mysys, prob_buff, betas);
	ising_enqueue(&mysys);
	ising_get_data(&mysys, mag_data);
	ising_get_states(&mysys, states_data);
	ising_free(&mysys);

	// Print states/data
	for (int k = 0; k < iter; k+=1)
	{
		printf("\e[1;1H\e[2J"); // clear screen

		for (int i = 0; i < sizeY; ++i)
		{
			for (int j = 0; j < sizeX; ++j)
			{
				char toprint = states_data[svec_length*k+i*sizeX+j];
				switch(toprint)
				{
					case 1: toprint = '#'; break;
					case -1: toprint = ' '; break;
					case 0: toprint = '+'; break;
					default: toprint = '?';
				}
				printf("%c ", toprint);
			}
			printf("\n");
		}

		printf("Iter: %d/%d\n", k, iter);
		printf("Beta: %f\n", betas[prob_buff*k/iter]);
		printf("Mag (GPU): %+5.4f", 2*(float)mag_data[k]/(sizeX*sizeY));
		printf("                                |\n");

		printf("[");
		for (int i = 0; i < 100; ++i)
		{
			printf("%c",(((float)i/100) < (0.5+(float)mag_data[k]/(sizeX*sizeY)))?'#':'-');
		}
		printf("]\n");
		printf("-1                                                0"
			"                                                 +1\n");

		// int sum = 0;
		// for (int i = 0; i < svec_length; ++i)
		// {
		// 	sum += states_data[svec_length*k+i];
		// }
		// printf("Mag (CPU): %d\n", mag_data[k]);

		if(_kbhit()) // break if key is pressed
		{
			getc(stdin);
			break;
		}

		if((k%(100))==0) k+=1024; // fast forward

		usleep(50000);
	}

	ising_profile();

	free(initial);
	free(states_data);
	free(mag_data);
	return 0;
}

/**
 Linux (POSIX) implementation of _kbhit().
 Morgan McGuire, morgan@cs.brown.edu
 */
int _kbhit() {
    static const int STDIN = 0;
    static int initialized = 0;

    if (! initialized) {
        // Use termios to turn off line buffering
        struct termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }

    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
