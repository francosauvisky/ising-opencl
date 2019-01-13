#include "ising.h"

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
	ising_configure(&mysys, initial, 2.2);
	ising_enqueue(&mysys);
	ising_get_data(&mysys, mag_data);
	ising_get_states(&mysys, states_data);
	ising_free(&mysys);

	// Print states/data
	for (int k = 0; k < iter; ++k)
	{
		printf("\e[1;1H\e[2J"); // clear screen

		for (int i = 0; i < sizeY; ++i)
		{
			for (int j = 0; j < sizeX; ++j)
			{
				printf("%c ", states_data[svec_length*k+i*sizeX+j]==1?'X':'.');
			}
			printf("\n");
		}
		printf("Mag: %d\n", mag_data[k]);
		usleep(80000);
	}

	free(initial);
	free(states_data);
	free(mag_data);
	return 0;
}
