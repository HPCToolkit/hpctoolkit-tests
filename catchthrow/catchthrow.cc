//
//// * BeginRiceCopyright *****************************************************
////
//// -----------------------------------
//// Part of HPCToolkit (hpctoolkit.org)
//// -----------------------------------
//// 
//// Copyright ((c)) 2002-2010, Rice University 
//// All rights reserved.
//// 
//// Redistribution and use in source and binary forms, with or without
//// modification, are permitted provided that the following conditions are
//// met:
//// 
//// * Redistributions of source code must retain the above copyright
////   notice, this list of conditions and the following disclaimer.
//// 
//// * Redistributions in binary form must reproduce the above copyright
////   notice, this list of conditions and the following disclaimer in the
////   documentation and/or other materials provided with the distribution.
//// 
//// * Neither the name of Rice University (RICE) nor the names of its
////   contributors may be used to endorse or promote products derived from
////   this software without specific prior written permission.
//// 
//// This software is provided by RICE and contributors "as is" and any
//// express or implied warranties, including, but not limited to, the
//// implied warranties of merchantability and fitness for a particular
//// purpose are disclaimed. In no event shall RICE or contributors be
//// liable for any direct, indirect, incidental, special, exemplary, or
//// consequential damages (including, but not limited to, procurement of
//// substitute goods or services; loss of use, data, or profits; or
//// business interruption) however caused and on any theory of liability,
//// whether in contract, strict liability, or tort (including negligence
//// or otherwise) arising in any way out of the use of this software, even
//// if advised of the possibility of such damage. 
//// 
//// ******************************************************* EndRiceCopyright *

// Usage:
//   catchthrow [iter [loop]]
//     Will run "loop" loops of "iter" iteractions of throwing and catching an exception
//     Default is loop = 25 and iter = 40000; you must specify iter if you want to specify loop
//     

#include <stdlib.h>
#include <iostream>

#define MAX_ITER   400000
#define MAX_LOOP   25

void    cputime(int);

int
main(int argc, char *argv[])
{
  int max = MAX_ITER;
  int loop = MAX_LOOP;

  if (argc == 2) {
    max = atoi(argv[1]);
  } else if (argc == 3) {
    max = atoi(argv[1]);
    loop = atoi(argv[2]);
  }
	
  std::cerr << "Will run " << loop << " loops of " << max << " iterations each" << std::endl;

  for (int j=0; j<loop; j++) {
    for (int i=0; i<max; i++) {
      try {
  throw "exception";
      } catch (const char *message) {
      }
    }
    std::cerr << "End loop " << j  << std::endl;
    // delay using CPU time
    cputime(10 * j);
  }
}

void
cputime(int k)
{
  int     i;      /* temp value for loop */
  int     j;      /* temp value for loop */
  volatile float  x;      /* temp variable for f.p. calculation */

  if(k == 0) {
    k = 10; 
  }   
  for (i = 0; i < k; i ++) {
    x = 0.0;
    for(j=0; j<1000000; j++) {
      x = x + 1.0;
    }   
  }   
  return;
}
