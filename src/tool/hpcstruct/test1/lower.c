#include <stdio.h>

volatile long ctr1 = 0;
long N1 = 1;

__attribute__((always_inline)) inline void a() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

__attribute__((always_inline)) inline void b() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

__attribute__((always_inline)) inline void c() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

__attribute__((always_inline)) inline void d() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

__attribute__((always_inline)) inline void e() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

__attribute__((always_inline)) inline void f() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

__attribute__((always_inline)) inline void g() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

void h() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

void i() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

void j() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

void k() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

void l() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

void m() { long i; for(i = 0; i < N1; i++) { ctr1++; } }

long
lower_half_pure_c(long ct, long num)
{
    ctr1 = ct;
    N1 = num;

    a(); b(); c(); d(); e(); f(); g();
    h(); i(); j(); k(); l(); m();

    return ctr1;
}
