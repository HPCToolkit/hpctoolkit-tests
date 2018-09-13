#include <omp.h>
#include <stdio.h>

__attribute__ ((constructor))
void a_constructor()
{
#pragma omp parallel 
	printf("hello from thread %d\n", omp_get_num_threads());
}

int main(int argc, char **argv)
{
	printf("hello from main\n");
	return 0;
}
