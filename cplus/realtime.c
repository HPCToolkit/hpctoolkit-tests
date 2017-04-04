/*
 *  Copyright (c) 2017, Rice University.
 *  See the file LICENSE for details.
 *
 *  Mark W. Krentel
 *  March 2017
 *
 *  This file provides support functions to create a POSIX
 *  CLOCK_REALTIME interrupt timer and to start and stop the timer.
 *  This is a process-wide (no threads) version of hpcrun's REALTIME
 *  sample source.
 *
 *  The header file for using these functions is part of monitor.h.
 */

#include <sys/types.h>
#include <err.h>
#include <error.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "monitor.h"

#define CLOCK_TYPE      CLOCK_REALTIME
#define NOTIFY_METHOD   SIGEV_SIGNAL
#define PROF_SIGNAL    (SIGRTMIN + 4)

static timer_t timerid;

//----------------------------------------------------------------------

int
rt_make_timer(rt_sighandler_t *handler)
{
    struct sigevent sev;
    struct sigaction act;
    int ret;

    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = NOTIFY_METHOD;
    sev.sigev_signo = PROF_SIGNAL;
    sev.sigev_value.sival_ptr = &timerid;

    ret = timer_create(CLOCK_TYPE, &sev, &timerid);
    if (ret != 0) {
	warn("timer_create failed");
	return ret;
    }

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = handler;
    act.sa_flags = SA_SIGINFO | SA_RESTART;

    ret = sigaction(PROF_SIGNAL, &act, NULL);
    if (ret != 0) {
	warn("sigaction failed");
	return ret;
    }

    return 0;
}

/*
 *  Period in micro-seconds.
 */
void
rt_start_timer(long usec)
{
    struct itimerspec start;
    long million = 1000000;

    memset(&start, 0, sizeof(start));
    start.it_value.tv_sec = usec / million;
    start.it_value.tv_nsec = 1000 * (usec % million);
    start.it_interval.tv_sec = usec / million;
    start.it_interval.tv_nsec = 1000 * (usec % million);

    timer_settime(timerid, 0, &start, NULL);
}

void
rt_stop_timer(void)
{
    struct itimerspec stop;

    memset(&stop, 0, sizeof(stop));

    timer_settime(timerid, 0, &stop, NULL);
}
