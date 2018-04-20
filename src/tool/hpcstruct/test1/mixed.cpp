//
//  Mixed C and C++ code, with inlined and non-inlined functions.
//  This is a test for how Symtab and hpcstruct deal with mangled,
//  demangled and inlined function names.
//

#include <stdio.h>

extern "C" {
long lower_half_pure_c(long, long);
}

long upper_half_c_plus_plus(long, long);

int
main(int argc, char **argv)
{
    long ctr, num;

    if (argc < 2 || sscanf(argv[1], "%ld", &num) < 1) {
        num = 10000;
    }
    num = num * num;

    ctr = lower_half_pure_c(0, num);
    ctr = upper_half_c_plus_plus(ctr, num);

    printf("counter = %ld\n", ctr);

    return 0;
}
