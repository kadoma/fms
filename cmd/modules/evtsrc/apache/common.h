/*
 * common.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *
 * Apache Service Event Source Module common include file
 *
 * This file contains common include files and defines used in many of
 * the apache service event source modules.
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <features.h>
/*Ben change*/

#include <stdio.h>							/* obligatory includes */
#include <stdlib.h>
#include <errno.h>

/* This block provides uintmax_t - should be reported to coreutils that this should be added to fsuage.h */
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>
//#define UINTMAX_MAX ((uintmax_t) -1)
/*Ben change*/

#include <limits.h>	/* This is assumed true, because coreutils assume it too */

#include <math.h>
/*Ben change*/

#include <strings.h>
#include <string.h>
/*Ben change*/

#include <unistd.h>
/*Ben change*/

/* GET_NUMBER_OF_CPUS is a macro to return 
   number of CPUs, if we can get that data.
   Use configure.in to test for various OS ways of
   getting that data
   Will return -1 if cannot get data
*/
#ifdef HAVE_SYSCONF__SC_NPROCESSORS_CONF 
#define GET_NUMBER_OF_CPUS() sysconf(_SC_NPROCESSORS_CONF)
#else
#define GET_NUMBER_OF_CPUS() -1
#endif

#include <sys/time.h>
#include <time.h>
/*Ben change*/

#include <sys/types.h>
/*Ben change*/

#include <sys/socket.h>
/*Ben change*/

#include <signal.h>
/*Ben change*/

/* GNU Libraries */
#include <getopt.h>
//#include "dirname.h"

#include <locale.h>
#include <sys/poll.h>
/*Ben change*/

/*
 *
 * Missing Functions
 *
 */

#ifndef HAVE_STRTOL
#define strtol(a,b,c) atol((a))
#endif

#ifndef HAVE_STRTOUL
#define strtoul(a,b,c) (unsigned long)atol((a))
#endif

/*
 *
 * Standard Values
 *
 */

enum {
	OK = 0,
	ERROR = -1
};

/* AIX seems to have this defined somewhere else */
enum {
	FALSE,
	TRUE
};

enum {
	STATE_OK,
	STATE_WARNING,
	STATE_CRITICAL,
	STATE_UNKNOWN,
	STATE_DEPENDENT
};

enum {
	DEFAULT_SOCKET_TIMEOUT = 10,	 /* timeout after 10 seconds */
	MAX_INPUT_BUFFER = 8192,	     /* max size of most buffers we use */
	MAX_HOST_ADDRESS_LENGTH = 256	 /* max size of a host address */
};

/*
 *
 * Internationalization
 *
 */
//#include "gettext.h"
//#define _(String) gettext (String)
//#if ! ENABLE_NLS
//# undef textdomain
//# define textdomain(Domainname) /* empty */
//# undef bindtextdomain
//# define bindtextdomain(Domainname, Dirname) /* empty */
//#endif

/* For non-GNU compilers to ignore __attribute__ */
#ifndef __GNUC__
#define __attribute__(x) /* do nothing */
#endif

#endif  /* _COMMON_H */
