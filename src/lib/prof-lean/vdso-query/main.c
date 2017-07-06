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
//  Verify operation of dso_symbols code, which extracts symbols from 
//  dynamic shared libraries.
//
//  John Mellor-Crummey
//  Rice University
//  July 2017
//  johnmc@rice.edu
//
//  Usage: ./nm-dso [load-module]
//     if load-module is omitted, it will print symbols for [vdso]
//

//******************************************************************************
// system includes
//******************************************************************************

#include <stdio.h>
#include <unistd.h>



//******************************************************************************
// local includes
//******************************************************************************

#include <vdso.h>



//******************************************************************************
// macros
//******************************************************************************

#define MAX_HOSTNAME_LEN 1024



//******************************************************************************
// private functions
//******************************************************************************


int 
main
(
)
{
	void *vdso_addr = vdso_segment_addr();
	if (vdso_addr) {
		size_t vdso_len = vdso_segment_len();

		char hostname[MAX_HOSTNAME_LEN];
		gethostname(hostname, MAX_HOSTNAME_LEN);
		
		char filename[MAX_HOSTNAME_LEN + 10];
		sprintf(filename, "%s-%s", VDSO_SEGMENT_NAME_SHORT, hostname);

		FILE *myfile = fopen(filename, "w");
		char module_name[1024];
		fwrite(vdso_addr, vdso_len, 1, myfile);
		fclose(myfile);
	}
	return 0;
}
