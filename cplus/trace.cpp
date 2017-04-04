//
//  Copyright (c) 2017, Rice University.
//  See the file LICENSE for details.
//
//  Mark W. Krentel
//  March 2017
//
//  This file is a proxy for hpcrun.so for LD_PRELOAD.  Turn on
//  REALTIME interrupts and make a C++ list of samples as a simple
//  trace profiler.
//
//  Implemented naively, this code will deadlock over the glibc
//  malloc() lock with high probability.  The trick is how to modify
//  building trace.so so that:
//
//   1. it does not depend on an outside libstdc++
//   2. internal malloc() is separate from application
//
//  Note: leave this file intact and make a copy to modify not to
//  deadlock.
//
//  Usage: LD_PRELOAD this file and export PERIOD as the number of
//  micro-seconds for REALTIME interrupts.
//

#include <sys/types.h>
#include <sys/time.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>

#include <iostream>
#include <list>
#include <sstream>
#include <string>

#include "monitor.h"

#define DEFAULT_PERIOD  2000

#define MAX_SAMPLES  50
#define MIN_SAMPLES  30

using namespace std;

class Sample;

typedef list <Sample *> SampleList;
typedef unsigned long ulong;

// For each sample, record the time in usec, stack depth and the IP
// address as a string (an excuse to use strings).
//
class Sample {
public:
    long   count;
    ulong  time;
    long   depth;
    string  func;

    Sample(long c, ulong t, long d, string & f) {
	count = c;
	time = t;
	depth = d;
	func = f;
    }
};

static SampleList sampleList;

static ulong  time0;
static ulong  stack_bottom;
static long  count;

//----------------------------------------------------------------------

// Get instruction pointer (ip) and stack pointer (sp) from user
// context.
//
static void
get_regs(ucontext_t *context, void **ip, void **sp)
{
    mcontext_t *mcontext = &(context->uc_mcontext);

#if defined(__x86_64__)
    *ip = (void *) mcontext->gregs[REG_RIP];
    *sp = (void *) mcontext->gregs[REG_RSP];

#elif defined(__powerpc__)
    *ip = (void *) mcontext->gregs[PPC_REG_PC];
    *sp = (void *) mcontext->gregs[PPC_REG_SP];

#else
    *ip = NULL;
    *sp = NULL;

#endif
}

// Returns: current time since the epoch in micro-seconds.
//
static ulong
get_usec(void)
{
    struct timeval now;

    gettimeofday(&now, NULL);

    return 1000000 * now.tv_sec + now.tv_usec;
}

//----------------------------------------------------------------------

// Thin the herd by deleting samples that are too close together.
// This is mostly an excuse to call delete.
//
static void
reduceSampleList()
{
    if (sampleList.size() <= MIN_SAMPLES) {
	return;
    }

    ulong delta = (get_usec() - time0)/MIN_SAMPLES;

    // last sample kept, never the end
    auto keep = sampleList.begin();
    auto it = keep;  ++it;

    // test if we want to keep 'it'
    while (it != sampleList.end()) {
	// always keep last sample
	auto next = it;  ++next;
	if (next == sampleList.end()) {
	    break;
	}

	// keep if time diff is large enough
	if ((*it)->time >= (*keep)->time + delta) {
	    keep = it;
	    ++it;
	}
	else {
	    delete *it;
	    sampleList.erase(it);
	    it = next;
	}
    }
}

static void
addSample(ucontext_t *context)
{
    ulong time = get_usec() - time0;
    count++;

    void *ip, *sp;
    get_regs(context, &ip, &sp);

    long depth = stack_bottom - (ulong) sp;

    stringstream buf;
    buf << "ip@" << ip;
    string func(buf.str());

    Sample * sample = new Sample(count, time, depth, func);

    sampleList.push_back(sample);

    if (sampleList.size() > MAX_SAMPLES) {
	reduceSampleList();
    }
}

static void
my_handler(int sig, siginfo_t *info, void *context)
{
    addSample((ucontext_t *) context);
}

//----------------------------------------------------------------------
//  Libmonitor callbacks
//----------------------------------------------------------------------

extern "C" {

void *
monitor_init_process(int *argc, char **argv, void *data)
{
    char *str = getenv("PERIOD");
    long period = DEFAULT_PERIOD;

    if (str != NULL && atol(str) > 0) {
	period = atol(str);
    }

    cout << "===>  init process:  period = " << period << " usec"
	 << "  <===\n\n";

    sampleList.clear();
    time0 = get_usec();
    stack_bottom = (ulong) monitor_stack_bottom();
    count = 0;

    ucontext_t context;
    if (getcontext(&context) != 0) {
	err(1, "getcontext failed");
    }
    addSample(&context);

    if (rt_make_timer(my_handler) != 0) {
	err(1, "timer create failed");
    }
    rt_start_timer(period);

    return NULL;
}

void
monitor_fini_process(int how, void *data)
{
    rt_stop_timer();

    cout << "\n===>  fini process:  total samples = " << count + 1
	 << "  <===\n";

    ucontext_t context;
    if (getcontext(&context) == 0) {
	addSample(&context);
    }
    else {
	warn("getcontext failed");
    }

    reduceSampleList();

    cout << "kept samples = " << sampleList.size() << "\n";

    for (auto it = sampleList.begin(); it != sampleList.end(); ++it) {
	Sample * sample = *it;

	cout << "index: " << sample->count
	     << "  time: " << sample->time
	     << "  depth: " << sample->depth
	     << "  " << sample->func << "\n";
    }
}

}  // extern "C"
