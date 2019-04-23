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

void h() {
    #pragma omp parallel num_threads(3)
    {
        loop2();
    }

}

void g() {
    h();
}

void f() {
    g();
}


void e() {
    #pragma omp parallel num_threads(3)
    {
        loop2();
        f();
    }
}

void d() {
    e();
}

void c() {
    #pragma omp parallel num_threads(2)
    {
        loop2();
        d();
    }
}

void b() {
    c();
}

void
a()
{
	#pragma omp parallel num_threads(2)
    {
        loop2();
        b();

    }
}

int
main()
{
    omp_set_nested(1);
    a();
    return 0;
}
