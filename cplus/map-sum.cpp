//
//  Copyright (c) 2017, Rice University.
//  See the file LICENSE for details.
//
//  Mark W. Krentel
//  March 2017
//
//  Application program to continually insert elements into a C++ map,
//  sum the elements and then clear the map.  This is just an excuse
//  to trigger a lot of calls to malloc() and free().
//
//  Usage: ./map-sum  [time-in-seconds]
//

#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <iostream>
#include <map>

#define SIZE  50000
#define DEFAULT_TIME  20

using namespace std;

typedef map <long, long> Lmap;

int
main(int argc, char **argv)
{
    struct timeval start, last, now;
    Lmap lmap;
    long n, len, sum;

    len = DEFAULT_TIME;
    if (argc > 1 && atol(argv[1]) > 0) {
	len = atol(argv[1]);
    }

    cout << "start\n";

    gettimeofday(&start, NULL);
    last = start;

    // run for len seconds
    for (;;) {
	lmap.clear();
	for (n = 1; n <= SIZE; n++) {
	    lmap[n] = n * n;
	}

	sum = 0;
	for (n = 1; n <= SIZE; n++) {
	    sum += lmap[n];
	}

	gettimeofday(&now, NULL);

	if (now.tv_sec > last.tv_sec) {
	    cout << "time: " << now.tv_sec - start.tv_sec
		 << "  sum: " << sum << "\n";
	    last = now;
	}

	if (now.tv_sec >= start.tv_sec + len) {
	    break;
	}
    }
    cout << "done\n";

    return 0;
}
