#include <omp.h>
#include <stdio.h>

#define N 40
#define T 4

long
fib(int n) {
    if (n < 2) return n;
    else return fib(n - 1) + fib(n - 2);
}

void
loop() {
    int j;
    for (j = 0; j < 1000000000; j += 2) j--;
}

void
loop2() {
    int j;
    for (j = 0; j < 100000000; j += 2) j--;
}


void e() {
    int i;
    for (i = 0; i < 3; i++) {
        #pragma omp parallel num_threads(2)
        {
            printf("\t\t\tTHIRD LEVEL REGION: %d\n", i);
            loop2();
        }
    }
}

void d() {
    e();
}

void c() {
    int i = 0;
    for (i = 0; i < 3; i++) {
        #pragma omp parallel num_threads(2)
        {
            printf("\t\tSECOND LEVEL REGION: %d\n", i);
            loop2();
            d();
        }
    }
}

void b() {
    c();
}

void
a() {
    int i = 0;
    for (i = 0; i < 3; i ++) {
        #pragma omp parallel num_threads(2)
        {
            printf("\tFIRST LEVEL REGION: %d\n", 0);
            loop2();
            b();
        }
    }
}

int
main() {
    omp_set_nested(1);
    a();
    return 0;
}
