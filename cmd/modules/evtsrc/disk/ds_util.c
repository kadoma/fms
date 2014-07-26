/*
 * ds_util.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *      ds_util.c
 */

#include <ctype.h>
#include "libdiskstatus.h"
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ds_impl.h"

bool ds_debug = 0;

void
dsprintf(const char *fmt, ...)
{
	va_list ap;

	if (!ds_debug)
		return;

	va_start(ap, fmt);
	(void) vprintf(fmt, ap);
	va_end(ap);
}

bool
set_err(const char *msg, ...)
{
	va_list ap;

	if (!ds_debug)
		return;

	va_start(ap, msg);
	(void) vprintf(msg, ap);
	va_end(ap);
	return false;
}

void
ddump(const char *label, const void *data, size_t length)
{
	int byte_count;
	int i;
#define	LINEBUFLEN 128
	char linebuf[LINEBUFLEN];
	char *linep;
	int bufleft, len;
	const char *start = data;

	if (!ds_debug)
		return;

	if (label != NULL)
		printf("%s\n", label);

	linep = linebuf;
	bufleft = LINEBUFLEN;

	for (byte_count = 0; byte_count < length; byte_count += i) {

		(void) snprintf(linep, bufleft, "0x%08x ", byte_count);
		len = strlen(linep);
		bufleft -= len;
		linep += len;

		/*
		 * Inner loop processes 16 bytes at a time, or less
		 * if we have less than 16 bytes to go
		 */
		for (i = 0; (i < 16) && ((byte_count + i) < length); i++) {
			(void) snprintf(linep, bufleft, "%02X", (unsigned int)
			    (unsigned char) start[byte_count + i]);

			len = strlen(linep);
			bufleft -= len;
			linep += len;

			if (bufleft >= 2) {
				if (i == 7)
					*linep = '-';
				else
					*linep = ' ';

				--bufleft;
				++linep;
			}
		}

		/*
		 * If i is less than 16, then we had less than 16 bytes
		 * written to the output.  We need to fixup the alignment
		 * to allow the "text" output to be aligned
		 */
		if (i < 16) {
			int numspaces = (16 - i) * 3;
			while (numspaces-- > 0) {
				if (bufleft >= 2) {
					*linep = ' ';
					--bufleft;
					linep++;
				}
			}
		}

		if (bufleft >= 2) {
			*linep = ' ';
			--bufleft;
			++linep;
		}

		for (i = 0; (i < 16) && ((byte_count + i) < length); i++) {
			int subscript = byte_count + i;
			char ch =  (isprint(start[subscript]) ?
			    start[subscript] : '.');

			if (bufleft >= 2) {
				*linep = ch;
				--bufleft;
				++linep;
			}
		}

		linebuf[LINEBUFLEN - bufleft] = 0;

		printf("%s\n", linebuf);

		linep = linebuf;
		bufleft = LINEBUFLEN;
	}

}

const char *
disk_status_errmsg(int error)
{
	switch (error) {
	case EDS_NOMEM:
		return ("memory allocation failure");
	case EDS_CANT_OPEN:
		return ("failed to open device");
	case EDS_NO_TRANSPORT:
		return ("no supported communication protocol");
	case EDS_NOT_SUPPORTED:
		return ("disk status information not supported");
	case EDS_NOT_SIMULATOR:
		return ("not a valid simulator file");
	case EDS_IO:
		return ("I/O error from device");
	default:
		return ("unknown error");
	}
}

int
ds_set_errno(disk_status_t *dsp, int error)
{
	dsp->ds_error = error;
	return (-1);
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// Returns 1 if machine is big endian, else zero.  This is a run-time
// rather than a compile-time function.  We could do it at
// compile-time but in principle there are architectures that can run
// with either byte-ordering.
int
isbigendian()
{
	short i=0x0100;
	char *tmp=(char *)&i;
	return *tmp;
}

// Returns true if region of memory contains non-zero entries
bool
nonempty(const void *data, int size)
{
	int i;
	for (i = 0; i < size; i++)
    		if (((const unsigned char *)data)[i])
      			return true;
  	return false;
}

// This routine converts an integer number of milliseconds into a test
// string of the form Xd+Yh+Zm+Ts.msec.  The resulting text string is
// written to the array.
void
MsecToText(unsigned int msec, char *txt)
{
	int start=0;
  	unsigned int days, hours, min, sec;

  	days       = msec/86400000U;
  	msec      -= days*86400000U;

  	hours      = msec/3600000U; 
  	msec      -= hours*3600000U;

  	min        = msec/60000U;
  	msec      -= min*60000U;

  	sec        = msec/1000U;
  	msec      -= sec*1000U;

  	if (days) {
    		txt += sprintf(txt, "%2dd+", (int)days);
    		start=1;
  	}

  	sprintf(txt, "%02d:%02d:%02d.%03d", (int)hours, (int)min, (int)sec, (int)msec);  
  	return;
}

// Used to warn users about invalid checksums. Called from atacmds.cpp.
void
checksumwarning(const char *string)
{
	printf("Warning! %s error: invalid SMART checksum.\n", string);
}

// A replacement for perror() that sends output to our choice of
// printing. If errno not set then just print message.
void
syserror(const char *message)
{  
	if (errno) {
    		// Get the correct system error message:
    		const char *errormessage=strerror(errno);
    
   		// Check that caller has handed a sensible string, and provide
   		// appropriate output. See perrror(3) man page to understand better.
    		if (message && *message)
      			printf("%s: %s\n", message, errormessage);
    		else
      			printf("%s\n", errormessage);
  	} else if (message && *message)
    		printf("%s\n",message);
  
  	return;
}

