#include <sys/time.h>


#define N (1 << 27)
int
main
(
)
{
	long long i;
	struct timeval tv;
	for(i=0;i<N;i++) {
          gettimeofday(&tv, 0);
	}
	return 0;
}
