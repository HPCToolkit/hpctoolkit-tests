//
//  Mixed C and C++ code, with inlined and non-inlined functions.
//  This is a test for how Symtab and hpcstruct deal with mangled,
//  demangled and inlined function names.
//

#include <stdio.h>

extern "C" {
long lower_half_pure_c(long, long);
}

volatile long ctr = 0;
long N = 1;

__attribute__((always_inline)) inline void n() { long i; for(i = 0; i < N; i++) { ctr++; } }

__attribute__((always_inline)) inline void o() { long i; for(i = 0; i < N; i++) { ctr++; } }

__attribute__((always_inline)) inline void p() { long i; for(i = 0; i < N; i++) { ctr++; } }

__attribute__((always_inline)) inline void q() { long i; for(i = 0; i < N; i++) { ctr++; } }

__attribute__((always_inline)) inline void r() { long i; for(i = 0; i < N; i++) { ctr++; } }

__attribute__((always_inline)) inline void s() { long i; for(i = 0; i < N; i++) { ctr++; } }

__attribute__((always_inline)) inline void t() { long i; for(i = 0; i < N; i++) { ctr++; } }

void u() { long i; for(i = 0; i < N; i++) { ctr++; } }

void v() { long i; for(i = 0; i < N; i++) { ctr++; } }

void w() { long i; for(i = 0; i < N; i++) { ctr++; } }

void x() { long i; for(i = 0; i < N; i++) { ctr++; } }

void y() { long i; for(i = 0; i < N; i++) { ctr++; } }

void z() { long i; for(i = 0; i < N; i++) { ctr++; } }

long
upper_half_c_plus_plus(long ct, long num)
{
    ctr = ct;
    N = num;

    n(); o(); p(); q(); r(); s(); t();
    u(); v(); w(); x(); y(); z();

    return ctr;
}

int
main(int argc, char **argv)
{
    long num;

    if (argc < 2 || sscanf(argv[1], "%ld", &num) < 1) {
        num = 10000;
    }
    num = num * num;

    ctr = lower_half_pure_c(0, num);
    ctr = upper_half_c_plus_plus(ctr, num);

    printf("counter = %ld\n", ctr);

    return 0;
}
