#define N 42

long a(int n)
{
	if (n < 2) return n;
	return a(n-1)+a(n-2);
}

long b(int n)
{
	if (n < 2) return n;
	return b(n-1)+b(n-2);
}


long c(int n)
{
	if (n < 2) return n;
	return c(n-1)+c(n-2);
}

int main(int argc, char **argv)
{
	a(N);
#pragma omp parallel 
	{
		b(N);
	}
	c(N);
	return 0;
}
