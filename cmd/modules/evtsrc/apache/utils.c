/*
 * utils.c
 *
 * Copyright (C) 2009 Inspur, Inc.  All rights reserved.
 * Copyright (C) 2009-2010 Fault Managment System Development Team
 *
 * Created on: Apr 07, 2010
 *      Author: Inspur OS Team
 *  
 * Description:
 * 
 * Library of useful functions for Apache Service Event Source Module
 */

#define LOCAL_TIMEOUT_ALARM_HANDLER

#include "common.h"
#include "utils.h"
#include <stdarg.h>
#include <limits.h>

#include <arpa/inet.h>

extern void print_usage (void);
extern const char *progname;

#define STRLEN 64
#define TXTBLK 128

/* **************************************************************************
 * max_state_alt(STATE_x, STATE_y)
 * compares STATE_x to  STATE_y and returns result based on the following
 * STATE_OK < STATE_DEPENDENT < STATE_UNKNOWN < STATE_WARNING < STATE_CRITICAL
 *
 * The main difference between max_state_alt and max_state it that it doesn't
 * allow setting a default to UNKNOWN. It will instead prioritixe any valid
 * non-OK state.
 ****************************************************************************/

int
max_state_alt (int a, int b)
{
	if (a == STATE_CRITICAL || b == STATE_CRITICAL)
		return STATE_CRITICAL;
	else if (a == STATE_WARNING || b == STATE_WARNING)
		return STATE_WARNING;
	else if (a == STATE_UNKNOWN || b == STATE_UNKNOWN)
		return STATE_UNKNOWN;
	else if (a == STATE_DEPENDENT || b == STATE_DEPENDENT)
		return STATE_DEPENDENT;
	else if (a == STATE_OK || b == STATE_OK)
		return STATE_OK;
	else
		return max (a, b);
}

long
deltime (struct timeval tv)
{
	struct timeval now;
	gettimeofday (&now, NULL);
	return (now.tv_sec - tv.tv_sec)*1000000 + now.tv_usec - tv.tv_usec;
}

void
strip (char *buffer)
{
	size_t x;
	int i;

	for (x = strlen (buffer); x >= 1; x--) {
		i = x - 1;
		if (buffer[i] == ' ' ||
				buffer[i] == '\r' || buffer[i] == '\n' || buffer[i] == '\t')
			buffer[i] = '\0';
		else
			break;
	}
	return;
}

/******************************************************************************
 *
 * Print perfdata in a standard format
 *
 ******************************************************************************/

char *perfdata (const char *label, long int val, const char *uom, int warnp,
		long int warn, int critp, long int crit, int minp,
		long int minv, int maxp, long int maxv)
{
	char *data = NULL;

	if (strpbrk (label, "'= "))
		asprintf (&data, "'%s'=%ld%s;", label, val, uom);
	else
		asprintf (&data, "%s=%ld%s;", label, val, uom);

	if (warnp)
		asprintf (&data, "%s%ld;", data, warn);
	else
		asprintf (&data, "%s;", data);

	if (critp)
		asprintf (&data, "%s%ld;", data, crit);
	else
		asprintf (&data, "%s;", data);

	if (minp)
		asprintf (&data, "%s%ld", data, minv);

	if (maxp)
		asprintf (&data, "%s;%ld", data, maxv);

	return data;
}

char *fperfdata (const char *label, double val, const char *uom, int warnp,
		double warn, int critp, double crit, int minp, double minv,
		int maxp, double maxv)
{
	char *data = NULL;

	if (strpbrk (label, "'= "))
		asprintf (&data, "'%s'=", label);
	else
		asprintf (&data, "%s=", label);

	asprintf (&data, "%s%f", data, val);
	asprintf (&data, "%s%s;", data, uom);

	if (warnp)
		asprintf (&data, "%s%f", data, warn);

	asprintf (&data, "%s;", data);

	if (critp)
		asprintf (&data, "%s%f", data, crit);

	asprintf (&data, "%s;", data);

	if (minp)
		asprintf (&data, "%s%f", data, minv);

	if (maxp) {
		asprintf (&data, "%s;", data);
		asprintf (&data, "%s%f", data, maxv);
	}

	return data;
}

