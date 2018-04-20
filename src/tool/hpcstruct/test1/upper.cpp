#include <stdio.h>

volatile long ctr2 = 0;
long N2 = 1;

__attribute__((always_inline)) inline void n() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

__attribute__((always_inline)) inline void o() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

__attribute__((always_inline)) inline void p() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

__attribute__((always_inline)) inline void q() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

__attribute__((always_inline)) inline void r() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

__attribute__((always_inline)) inline void s() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

__attribute__((always_inline)) inline void t() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

void u() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

void v() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

void w() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

void x() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

void y() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

void z() { long i; for(i = 0; i < N2; i++) { ctr2++; } }

long
upper_half_c_plus_plus(long ct, long num)
{
    ctr2 = ct;
    N2 = num;

    n(); o(); p(); q(); r(); s(); t();
    u(); v(); w(); x(); y(); z();

    return ctr2;
}
