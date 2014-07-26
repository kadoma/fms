/*
 * utils.h
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 *
 * Header file for nagios plugins utils.c
 * This file should be included in all plugins
 *
 * The purpose of this package is to provide safer alternatives to C
 * functions that might otherwise be vulnerable to hacking. This
 * currently includes a standard suite of validation routines to be sure
 * that an string argument acually converts to its intended type and a
 * suite of string handling routine that do their own memory management
 * in order to resist overflow attacks. In addition, a few functions are
 * provided to standardize version and error reporting across the entire
 * suite of plugins.
 */

/* now some functions etc are being defined in ../lib/utils_base.c */
//#include "utils_base.h"

#ifndef _UTILS_H
#define _UTILS_H

#ifdef NP_EXTRA_OPTS
/* Include extra-opts functions if compiled in */
#include "extra_opts.h"
#else
/* else, fake np_extra_opts */
#define np_extra_opts(acptr,av,pr) av
#endif

#include <sys/time.h>
#include <time.h>
/*Ben add*/

double delta_time (struct timeval tv);
long deltime (struct timeval tv);

/* Handle strings safely */

void strip (char *);
char *strscpy (char *, const char *);
char *strnl (char *);
char *strpcpy (char *, const char *, const char *);
char *strpcat (char *, const char *, const char *);

int max_state (int a, int b);
int max_state_alt (int a, int b);

#define max(a,b) (((a)>(b))?(a):(b))
#define min(a,b) (((a)<(b))?(a):(b))

char *perfdata (const char *, long int, const char *, int, long int, int,
		long int, int, long int, int, long int);

char *fperfdata (const char *, double, const char *, int, double, int, double,
		int, double, int, double);

#endif  /* _UTILS_H */
