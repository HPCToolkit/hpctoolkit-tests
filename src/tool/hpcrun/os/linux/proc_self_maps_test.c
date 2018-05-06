// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2018, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *


//******************************************************************************
// global includes
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "proc_self_maps.h"



//******************************************************************************
// types
//******************************************************************************

typedef struct proc_self_maps_seg_s {
	proc_self_maps_segment_t s;
	struct proc_self_maps_seg_s *next;
} proc_self_maps_seg_t;



//******************************************************************************
// local variables
//******************************************************************************

static proc_self_maps_seg_t *head;



//******************************************************************************
// private operations
//******************************************************************************

static void
push(proc_self_maps_segment_t *s)
{
   proc_self_maps_seg_t *e = malloc(sizeof(proc_self_maps_seg_t));
   memcpy(&(e->s), s, sizeof(*s));
   e->next = head;
   head = e;
}


static proc_self_maps_segment_t *
pop()
{
   proc_self_maps_seg_t *e = head;
   if (e) head = e->next; 
   return &e->s;
}

static int 
callback(proc_self_maps_segment_t *s, void *arg)
{
    proc_self_maps_segment_print(s);
    push(s);
    return 0;
}



//******************************************************************************
// interface operations
//******************************************************************************

int
main(int argc, char **argv)
{
  printf("dump /proc/self/maps\n");

  char cmd[1024];
  sprintf(cmd, "cat /proc/%ld/maps", (long) getpid());
  system(cmd);
  
  printf("\niteration over segments in /proc/self/maps\n");
  proc_self_maps_segment_iterate(callback, 0);

  printf("\nlooking up segments in reverse order\n");
  proc_self_maps_segment_t *e;
  for(e = pop(); e; e = pop()) {
    proc_self_maps_segment_t s;
    char *addr = 1 + (char *) e->start; 
    proc_self_maps_address_to_segment(addr, &s);
    printf("Lookup %p --> ", addr); 
    proc_self_maps_segment_print(&s);
  }

  return 0;
}
