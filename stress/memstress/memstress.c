/*
 *  Copyright (c) 2020, Rice University.
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
 *  ----------------------------------------------------------------------
 *
 *  Memory stress test.  Create multiple threads, each thread runs a
 *  loop of: pull an item off a common work list, verify the data is
 *  not corrupt, free() the data and malloc() a new region.
 *
 *  Note: the thread that verifies and deletes an item is not
 *  necessarily the thread that created it.
 *
 *  This is a stress test for hpcrun for two things:
 *
 *    1. That general hpcrun doesn't deadlock by taking a sample
 *    inside malloc() or free().
 *
 *    2. That hpcrun MEMLEAK source doesn't deadlock or corrupt
 *    memory, and rough estimate of overhead.
 *
 *  TODO: Vary the size of regions that are malloc()ed.
 *
 *  Usage:  memstress  [time in secs]  [num threads]
 */

#include <sys/types.h>
#include <sys/time.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

#define MAX_THREADS  550
#define PROG_TIME     30
#define NUM_THREADS    4
#define SIZE         200

int prog_time;
int num_threads;

struct thread_info {
    pthread_t self;
    int tid;
};

struct thread_info thread[MAX_THREADS];

struct mem_info {
    long  a[SIZE];
};

struct mem_entry {
    struct mem_info * info;
    struct mem_entry * next;
};

/* fifo list, insert at tail, remove from head */
struct mem_entry * list_head = NULL;
struct mem_entry * list_tail = NULL;

/* protects the mem entries list */
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct timeval start;

long num_malloc = 0;
long num_free = 0;

//----------------------------------------------------------------------

/*
 *  Insert at list tail and delete from list head.  These functions
 *  assume the list is already locked.
 */
void
list_insert(struct mem_entry * me)
{
    me->next = NULL;

    if (list_tail == NULL) {
	list_head = me;
	list_tail = me;
	return;
    }

    list_tail->next = me;
    list_tail = me;
}

struct mem_entry *
list_delete(void)
{
    if (list_head == NULL) {
	return NULL;
    }

    struct mem_entry * me = list_head;

    list_head = list_head->next;

    if (list_head == NULL) {
	list_tail = NULL;
    }

    return me;
}

//----------------------------------------------------------------------

/*
 *  Main unit of work: remove one item from head of list (with mutex),
 *  verify the contents and free, add one new item to list.
 */
void
do_malloc(int tid, int do_delete)
{
    long val;
    int i;

    /*
     * create new entry for the list
     */
    struct mem_entry * new_me = (struct mem_entry *) malloc(sizeof(struct mem_entry));
    struct mem_info * new_mi = (struct mem_info *) malloc(sizeof(struct mem_info));

    if (new_me == NULL || new_mi == NULL) {
	err(1, "malloc mem entry/info failed");
    }

    /*
     * the value to write is the thread id
     */
    val = tid;
    for (i = 0; i < SIZE; i++) {
	new_mi->a[i] = val;
    }

    new_me->info = new_mi;

    /*
     * put new_me on the list and remove head if do_delete
     */
    struct mem_entry * old_me = NULL;
    struct mem_info * old_mi = NULL;

    pthread_mutex_lock(&mtx);

    if (do_delete) {
	old_me = list_delete();
	num_free += 2;
    }
    list_insert(new_me);
    num_malloc += 2;

    pthread_mutex_unlock(&mtx);

    if (old_me == NULL) {
	return;
    }

    /*
     * verify old entry and free
     */
    old_mi = old_me->info;
    val = old_mi->a[0];

    for (i = 1; i < SIZE; i++) {
	if (old_mi->a[i] != val) {
	    errx(1, "memory corruption: val %ld != %ld",
		 old_mi->a[0], old_mi->a[i]);
	}
    }

    free(old_me);
    free(old_mi);
}

//----------------------------------------------------------------------

/*
 *  Main loop per thread.  Loop of do_malloc() for prog_time seconds.
 *  Thread 0 reports on status.
 */
void
memstress(int tid)
{
    struct timeval now, last;
    int num;

    last = start;

    /*
     * seed the list with 50 entries per thread
     */
    for (num = 1; num <= 50; num++) {
	do_malloc(tid, 0);
    }

    for (;;) {
	for (num = 1; num <= 100; num++) {
	    do_malloc(tid, 1);
	}

        gettimeofday(&now, NULL);

	if (tid == 0 && now.tv_sec > last.tv_sec) {
	    printf("time: %4ld    malloc: %12ld    free: %12ld\n",
		   now.tv_sec - start.tv_sec, num_malloc, num_free);
	    last = now;
	}

        if (now.tv_sec >= start.tv_sec + prog_time) {
	    break;
	}
    }
}

//----------------------------------------------------------------------

void *
my_thread(void *data)
{
    struct thread_info *ti = data;
    int tid = ti->tid;

    memstress(tid);

    return NULL;
}

//----------------------------------------------------------------------

/*
 *  Program args: prog_time, num_threads.
 */
int
main(int argc, char **argv)
{
    struct thread_info *ti;
    int num;

    if (argc < 2 || sscanf(argv[1], "%d", &prog_time) < 1) {
	prog_time = PROG_TIME;
    }
    if (argc < 3 || sscanf(argv[2], "%d", &num_threads) < 1) {
	num_threads = NUM_THREADS;
    }
    if (num_threads > MAX_THREADS) {
	num_threads = MAX_THREADS;
    }

    printf("memstress: prog_time: %d   num_threads: %d\n",
	   prog_time, num_threads);

    list_head = NULL;
    list_tail = NULL;

    gettimeofday(&start, NULL);

    /*
     * launch side threads
     */
    printf("\nlaunching threads ...\n");
    for (num = 1; num < num_threads; num++) {
	ti = &thread[num];
	ti->tid = num;
        if (pthread_create(&ti->self, NULL, my_thread, ti) != 0) {
            err(1, "pthread_create num %d failed", num);
	}
    }

    /*
     * main thread, same work as side threads
     */
    ti = &thread[0];
    ti->tid = 0;
    ti->self = pthread_self();
    my_thread(ti);

    /*
     * join with threads
     */
    printf("\njoining threads ...\n");
    for (num = 1; num < num_threads; num++) {
	ti = &thread[num];
	if (pthread_join(ti->self, NULL) != 0) {
	    warnx("pthread_join num %d failed", num);
	}
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    double diff = (now.tv_sec - start.tv_sec)
	+ ((double) (now.tv_usec - start.tv_usec)) / 1000000.0;

    if (diff < 0.001) { diff = 0.001; }

    double rate = ((double) num_malloc) / diff;

    printf("total malloc:  %g    rate:  %g / sec\n",
	   (double) num_malloc, rate);

    printf("done\n");

    return 0;
}
