/*
 *  Copyright (c) 2017, Rice University.
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
 * ----------------------------------------------------------------------
 *
 *  This file is a stripped-down version of libmonitor's main.c to
 *  provide just the init and fini process callbacks.  See monitor.h
 *  for the list of callbacks and support functions.
 *
 *  Notes:
 *  1. Only implements the dynamic case with __libc_start_main().
 *
 *  2. Supports x86_64 and powerpc (different start main args).
 *
 *  3. Assumes the application program returns from main() and does
 *  not call exit() or _exit(), or else the fini process callback
 *  won't happen.
 *
 *  4. All callbacks must be implemented, no weak symbols.
 */

#include <sys/types.h>
#include <alloca.h>
#include <err.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "monitor.h"

#ifndef RTLD_NEXT
#define RTLD_NEXT  ((void *) -1l)
#endif

#if defined(__powerpc__)

#define AUXVEC_ARG   , auxvec
#define AUXVEC_DECL  , void * auxvec

#define START_MAIN_PARAM_LIST 		\
    int  argc,				\
    char **argv,			\
    char **envp,			\
    void *auxp,				\
    void (*rtld_fini)(void),		\
    void **stinfo,			\
    void *stack_end

static void *new_stinfo[4];

#else  /* default __libc_start_main() args */

#define AUXVEC_ARG
#define AUXVEC_DECL

#define START_MAIN_PARAM_LIST 		\
    int  (*main)(int, char **, char **),  \
    int  argc,				\
    char **argv,			\
    void (*init)(void),			\
    void (*fini)(void),			\
    void (*rtld_fini)(void),		\
    void *stack_end

#endif

typedef int start_main_fcn_t(START_MAIN_PARAM_LIST);
typedef int main_fcn_t(int, char **, char **  AUXVEC_DECL );

static start_main_fcn_t  *real_start_main = NULL;
static main_fcn_t  *real_main = NULL;

static void *stack_bottom = NULL;

//----------------------------------------------------------------------

void *
monitor_stack_bottom(void)
{
    return stack_bottom;
}

static int
monitor_main(int argc, char **argv, char **envp  AUXVEC_DECL )
{
    void *user_data;
    int ret;

    stack_bottom = alloca(8);
    strncpy(stack_bottom, "stakbot", 8);

    user_data = monitor_init_process(&argc, argv, NULL);

    ret = (*real_main)(argc, argv, envp  AUXVEC_ARG );

    monitor_fini_process(1, user_data);

    return ret;
}

int
__libc_start_main(START_MAIN_PARAM_LIST)
{
    const char *err_str;

    dlerror();
    real_start_main = dlsym(RTLD_NEXT, "__libc_start_main");
    err_str = dlerror();

    if (real_start_main == NULL) {
	errx(1, "dlsym failed: %s", err_str);
    }

#if defined(__powerpc__)
    real_main = stinfo[1];
    memcpy(new_stinfo, stinfo, sizeof(new_stinfo));
    new_stinfo[1] = &monitor_main;

    (*real_start_main)(argc, argv, envp, auxp, rtld_fini,
                       new_stinfo, stack_end);
#else
    real_main = main;

    (*real_start_main)(monitor_main, argc, argv, init, fini,
                       rtld_fini, stack_end);
#endif

    return 0;
}
