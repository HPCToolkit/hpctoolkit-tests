#define N (1 << 29)

volatile long count;

void fn()
{
	count++; 
}


void loop()
{
	long i;
	for(i = 0; i < N; i++) fn();
}


int
main(int argc, char **argv)
{
#pragma omp parallel num_threads(2)
	loop();
	return 0;
}
