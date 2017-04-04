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
 *  This file is a stripped-down version of libmonitor's monitor.h
 *  plus the realtime support functions.
 */

#ifndef  _MONITOR_H_
#define  _MONITOR_H_

#include <sys/types.h>
#include <signal.h>

typedef void rt_sighandler_t(int, siginfo_t *, void *);

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  Callback functions for the client to override.
 */
extern void *monitor_init_process(int *argc, char **argv, void *data);
extern void monitor_fini_process(int how, void *data);

/*
 *  Monitor support functions.
 */
extern void *monitor_stack_bottom(void);

/*
 *  Support functions from realtime.c.
 */
int  rt_make_timer(rt_sighandler_t *);
void rt_start_timer(long);
void rt_stop_timer();

#ifdef __cplusplus
}
#endif

#endif
