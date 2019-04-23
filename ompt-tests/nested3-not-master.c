#include <omp.h>
#include <stdio.h>

#define N 40
#define T 4

long
fib(int n) {
    if (n < 2) return n;
    else return fib(n-1) + fib(n-2);
}

void
loop(){
    int j;
    for(j=0;j<1000000000;j+=2) j--;
}

void
loop2(){
    int j;
    for(j=0;j<100000000;j+=2) j--;
}

void
a()
{
	#pragma omp parallel num_threads(2)
    {
        if (omp_get_thread_num() != 0){
            loop2();
        }

		#pragma omp parallel num_threads(2)
        {
            if (omp_get_thread_num() != 0){
                loop2();
            }

	    	#pragma omp parallel num_threads(3)
            {

                if (omp_get_thread_num() != 0){
                    loop2();
                }

				#pragma omp parallel num_threads(3)
                {
                    if (omp_get_thread_num() != 0) {
                        loop2();
                    }
                }
	   		}
        }
    }
}

int
main()
{
    omp_set_nested(1);
    a();
    return 0;
}
