// -*-Mode: C++;-*- // technically C99

// * BeginRiceCopyright *****************************************************
//
// $HeadURL: https://outreach.scidac.gov/svn/hpctoolkit/trunk/src/tool/hpcrun/T1/fib.c $
// $Id: fib.c 2770 2010-03-06 02:54:44Z tallent $
//
// -----------------------------------
// Part of HPCToolkit (hpctoolkit.org)
// -----------------------------------
// 
// Copyright ((c)) 2002-2010, Rice University 
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

#include <stdio.h>

#include "fib.h"
#include "fib_thread.h"

int
main(int argc, char **argv)
{
  int n;

  if (argc < 2 || sscanf(argv[1], "%d", &n) < 1)
    n = 14;

  printf("n = %d, ans = %ld\n", n, fib(n));
  
  // first attempt
  std::promise<int> p;
  auto pr = p.get_future();
  fib_thread(n, std::move(p));
  
  printf("1. c++ thread version = %ld\n", pr.get());

  // second attempt
  std::promise<int> q;
  auto pq = q.get_future();
  fib_thread(n, std::move(q));
  
  printf("2. c++ thread version = %ld\n", pq.get());

  return (0);
}
