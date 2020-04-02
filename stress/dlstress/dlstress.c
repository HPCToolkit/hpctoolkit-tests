/*
 *  Copyright (c) 2019-2020, Rice University.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *  * Neither the name of Rice University (RICE) nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  This software is provided by RICE and contributors "as is" and any
 *  express or implied warranties, including, but not limited to, the
 *  implied warranties of merchantability and fitness for a particular
 *  purpose are disclaimed. In no event shall RICE or contributors be
 *  liable for any direct, indirect, incidental, special, exemplary, or
 *  consequential damages (including, but not limited to, procurement of
 *  substitute goods or services; loss of use, data, or profits; or
 *  business interruption) however caused and on any theory of liability,
 *  whether in contract, strict liability, or tort (including negligence
 *  or otherwise) arising in any way out of the use of this software, even
 *  if advised of the possibility of such damage.
 *
 *  Mark W. Krentel
 *  Rice University
 *  March 2020
 *
 * ----------------------------------------------------------------------
 *
 *  This program runs one or two threads and runs a loop of dlopen(),
 *  dlclose and optionally dlsym in each thread.  This is a stress
 *  test for hpcrun designed to cause trouble with the dlopen reader-
 *  writer lock and dl_iterate_phdr().
 *
 *  Usage:  dlstress [ <program-time> | mult | single | nosym ]*
 *
 *   program-time -- program time in seconds
 *   mult   -- run with multiple (2) threads
 *   single -- run with single (1) thread
 *   nosym  -- do not call dlsym()
 */

#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dlfcn.h>
#include <pthread.h>

#define LIB_DIR  "/usr/lib64"

#define MAX_LIBS  100
#define NUM_SUM_FUNCS  20
#define PROGRAM_TIME   30

typedef double (sum_func_t) (long);

char *base[MAX_LIBS] = {
    "libICE",
    "libOpenGL",
    "libabrt",
    "libasm",
    "libasprintf",
    "libatm",
    "libatomic_ops",
    "libattr",
    "libaudit",
    "libao",
    "libcap",
    "libcurl",
    "libdrm",
    "libdw",
    "libelf",
    "libffi",
    "libform",
    "libm",
    "libmagic",
    "libncurses",
    "libpam",
    "libssl",
    "libtar",
    "libxml2",
    "libz",
    NULL,
};

char *name[MAX_LIBS] = {
    NULL,
};

struct thread_args {
    char *label;
    int  index;
    int  start;
    int  len;
};

int num_libs = 0;

int program_time;
int mult_thread;
int do_dlsym;

struct timeval start;

//----------------------------------------------------------------------

/*
 * Create name[] array from base names.  Resolve version names by
 * taking the longest file name matching the glob pattern.
 * For example: libz --> /usr/lib64/libz.so.1.2.11
 */
void
mk_lib_array(void)
{
    glob_t globbuf;
    char patn[500];
    int i, j;

    printf("searching for libs ...\n");

    num_libs = 0;
    for (i = 0; base[i] != NULL; i++) {
	sprintf(patn, "%s/%s.so*", LIB_DIR, base[i]);

	int ret = glob(patn, 0, NULL, &globbuf);

	if (ret != 0 || globbuf.gl_pathc < 1) {
	    printf("failure:  %s\n", patn);
	    goto next_lib;
	}

	// select longest file name among all matches.
	// this is the file most likely to work.
	char *lib = globbuf.gl_pathv[0];
	int len = strlen(lib);

	for (j = 1; j < globbuf.gl_pathc; j++) {
	    if (strlen(globbuf.gl_pathv[j]) > len) {
		lib = globbuf.gl_pathv[j];
		len = strlen(lib);
	    }
	}

	// test if we can dlopen the library
	void * handle = dlopen(lib, RTLD_LAZY);

	if (handle == NULL) {
	    printf("failure:  %s\n", lib);
	    goto next_lib;
	}

	// success: add it to the name[] array
	name[num_libs] = strdup(lib);
	num_libs++;
	dlclose(handle);
	printf("success:  %s\n", lib);

    next_lib:
	globfree(&globbuf);
    }
    printf("num libs:  %d\n\n", num_libs);
}

//----------------------------------------------------------------------

/*
 * Main thread uses name[0, ..., N/2 - 1], side thread uses N/2 ... N - 1,
 * where N = num_libs.
 */
void
mk_thread_args(struct thread_args * main_args, struct thread_args * side_args)
{
    if (num_libs < 8) {
	errx(1, "not enough available libraries");
    }

    main_args->label = "main";
    main_args->index = 1;
    main_args->start = 0;
    main_args->len = num_libs/2;

    side_args->label = "side";
    side_args->index = 2;
    side_args->start = num_libs/2;
    side_args->len = num_libs - main_args->len;
}

//----------------------------------------------------------------------

/*
 * Main loop of dlopen(), dlclose and dlsym run by both threads.
 * Report on the number of calls.
 */
void
do_loop(struct thread_args * args)
{
    struct timeval now, last;
    void * handle[MAX_LIBS];
    long num;

    long cur_open = 0;
    long cur_sym = 0;
    long total_open = 0;
    long total_sym = 0;
    long total_err = 0;
    long num_open = 0;

    last = start;

    for (num = 1;; num++)
    {
	double sum = 0.0;
	int k;

	/* open libraries, vary the number to open so they don't
	 * always go to the same address.
	 */
	for (k = 0; k < num_open; k++) {
	    handle[k] = dlopen(name[args->start + k], RTLD_LAZY);
	    if (handle[k] == NULL) {
		total_err++;
	    }
	}
	cur_open += num_open;
	total_open += num_open;

	if (do_dlsym) {
	    /*
	     * dlsym funcs in libsum.so and run cycles
	     */
	    char buf[500];
	    sprintf(buf, "./libsum%d.so", args->index);
	    void *sum_handle = dlopen(buf, RTLD_LAZY);

	    for (k = 0; k < NUM_SUM_FUNCS; k++) {
		sprintf(buf, "sum_%d_%d", args->index, k);
		sum_func_t * sum_func = dlsym(sum_handle, buf);
		sum += (* sum_func) (5000);
	    }
	    dlclose(sum_handle);

	    cur_open += 1;
	    total_open += 1;
	    cur_sym += NUM_SUM_FUNCS;
	    total_sym += NUM_SUM_FUNCS;
	}
	else {
	    /*
	     * no dlsym, simple cycles
	     */
	    for (k = 1; k <= 250000; k++) {
		sum += (double) k;
	    }
	}

	/*
	 * close in reverse order
	 */
	for (k = num_open - 1; k >= 0; k--) {
	    if (handle[k] != NULL) {
		dlclose(handle[k]);
	    }
	}

	num_open = (num_open + 1) % args->len;

        gettimeofday(&now, NULL);

        if (now.tv_sec > last.tv_sec) {
            printf("%s:  time: %3ld   dlopen: %6ld  (%ld)   dlsym: %6ld  (%ld)"
		   "   err: %ld   sum = %g\n",
                   args->label, now.tv_sec - start.tv_sec, cur_open, total_open,
		   cur_sym, total_sym, total_err, sum);
            last = now;
	    cur_open = 0;
	    cur_sym = 0;
        }

        if (now.tv_sec >= start.tv_sec + program_time) {
	    printf("%s:  done\n", args->label);
            break;
        }
    }
}

//----------------------------------------------------------------------

void *
side_thread(void *data)
{
    do_loop(data);
    return NULL;
}

//----------------------------------------------------------------------

/*
 * Args:
 *  program-time  (in seconds),
 *  'mult', 'single', 'nosym'.
 */
void
parse_args(int argc, char **argv)
{
    program_time = PROGRAM_TIME;
    mult_thread = 1;
    do_dlsym = 1;

    for (int k = 1; k < argc; k++) {
	if (isdigit(argv[k][0])) {
	    program_time = atoi(argv[k]);
	}
	else if (strncmp(argv[k], "multiple", 4) == 0) {
	    mult_thread = 1;
	}
	else if (strncmp(argv[k], "single", 4) == 0) {
	    mult_thread = 0;
	}
	else if (strncmp(argv[k], "nosym", 4) == 0) {
	    do_dlsym = 0;
	}
	else {
	    errx(1, "unknown flag: %s", argv[k]);
	}
    }
}

//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    struct thread_args main_args, side_args;
    pthread_t td;

    parse_args(argc, argv);

    printf("dlstress: loop of dlopen, dlclose and dlsym\n"
	   "program time: %d  %s thread,  %s\n\n",
	   program_time,
	   (mult_thread) ? "mult" : "single",
	   (do_dlsym) ? "with dlsym" : "no dlsym");

    mk_lib_array();

    mk_thread_args(&main_args, &side_args);

    gettimeofday(&start, NULL);

    if (mult_thread) {
	if (pthread_create(&td, NULL, side_thread, &side_args) != 0) {
	    err(1, "pthread_create failed");
	}
    }

    do_loop(&main_args);

    if (mult_thread) {
	printf("\nwaiting on pthread_join ...\n");
	pthread_join(td, NULL);
    }

    printf("done\n");

    return 0;
}
