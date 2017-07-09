//
//  Copyright (c) 2017, Rice University.
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//  
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  
//  * Neither the name of Rice University (RICE) nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//  
//  This software is provided by RICE and contributors "as is" and any
//  express or implied warranties, including, but not limited to, the
//  implied warranties of merchantability and fitness for a particular
//  purpose are disclaimed. In no event shall RICE or contributors be
//  liable for any direct, indirect, incidental, special, exemplary, or
//  consequential damages (including, but not limited to, procurement of
//  substitute goods or services; loss of use, data, or profits; or
//  business interruption) however caused and on any theory of liability,
//  whether in contract, strict liability, or tort (including negligence
//  or otherwise) arising in any way out of the use of this software, even
//  if advised of the possibility of such damage.
//
//----------------------------------------------------------------------
//
//  Record /proc/self/maps and [vdso] to support regression testing of
//  runtime environment analysis modules. 
//
//  John Mellor-Crummey
//  Rice University
//  July 2017
//  johnmc@rice.edu
//
//  Usage: ./vdso-query 


//******************************************************************************
// system includes
//******************************************************************************

#include <stdio.h>
#include <unistd.h>
#include <string.h>



//******************************************************************************
// local includes
//******************************************************************************

#include <vdso.h>



//******************************************************************************
// macros
//******************************************************************************

#define BUFSIZE 1024
#define LONGNAME 2048



//******************************************************************************
// private functions
//******************************************************************************

char *myhostname = 0;

char *
hostname()
{
  if (!myhostname) {
    char hostname[BUFSIZE];
    gethostname(hostname, BUFSIZE);
    myhostname = strdup(hostname);
  }
  return myhostname;
}

char *
getfilename(char *prefix)
{
   static char filename[LONGNAME];
   char *host = hostname();
   sprintf(filename, "%s-%s", prefix, host);
   return filename;
}

void
copymaps()
{
	FILE *maps = fopen("/proc/self/maps", "r");
	char *filename = getfilename("maps");
	FILE *mymaps = fopen(filename, "w");
	int count;
	do {
		char buffer[BUFSIZE];
		count = fread(buffer, 1, BUFSIZE, maps); 
		if (count == 0) break;
		fwrite(buffer, 1, count, mymaps);
	} while(count == BUFSIZE);
	fclose(mymaps);
}

void
copyvdso()
{
	void *vdso_addr = vdso_segment_addr();
	if (vdso_addr) {
		size_t vdso_len = vdso_segment_len();

		char *filename = getfilename(VDSO_SEGMENT_NAME_SHORT);
		FILE *myfile = fopen(filename, "w");
		fwrite(vdso_addr, vdso_len, 1, myfile);
		fclose(myfile);
	}
}


int 
main
(
)
{
	copymaps();
 	copyvdso();
	return 0;
}
