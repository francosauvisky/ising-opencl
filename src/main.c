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

	for (int i = 0; i < svec_length; ++i)
	{
		initial[i] = (rand() < (RAND_MAX/2))? 1 : -1;
	}

	ising_init();
	system_t mysys = ising_new();
	ising_configure(&mysys, initial, 3.0);
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
				printf("%c ", states_data[svec_length*k+i*sizeX+j]==1?'#':'-');
			}
			printf("\n");
		}

		printf("Iter: %d/%d\n", k, iter);
		printf("Mag (GPU): %5.4f\n", (float)mag_data[k]/(sizeX*sizeY));

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

		usleep(100000);
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
