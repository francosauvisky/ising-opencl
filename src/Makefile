ising.out: main.c ising.h ising.o ising-param.h ising.cl
	gcc main.c -lm -lOpenCL -o ising.out ising.o

ising.o: ising.c ising.h ising-param.h
	gcc ising.c -lm -lOpenCL -c -o ising.o
