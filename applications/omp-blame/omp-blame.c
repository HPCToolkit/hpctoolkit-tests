#include <stdio.h>
#include <omp.h>
#include <pthread.h>
#include <unistd.h>

#include "timer.h"

void mysleep(int sec)
{
	timer_start();
	for(;;) {
	   if (timer_elapsed() > sec) break;
	}
}

int
main(int argc, char **argv)
{

#pragma omp parallel num_threads(4)
{
	int tid = omp_get_thread_num();
	if (tid == 0) { 
	    mysleep(5);
	}
}

	return 0;
}
