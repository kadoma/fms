/*
 * file: 	fm_rtdectc
 * describe:	route error dect
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "fm_rtdect.h"

#define RT_TABLE	"/proc/net/route"

/*
 * route_check
 *  dect route table error
 *  open route table in /proc/net/route, and
 *  check if all network is reachable. It's done
 *  by do 'and' operation on all "Destination" 
 *  field, if the result is zero, all network is
 *  reachable.
 *
 * Input 
 *  void
 *
 * Return value
 *  0		- if route table is OK
 *  1		- if route table has some error
 *  other	- if error happen
 */
int route_check()
{
	FILE		*fd_rt;
	char		line_buf[LINE_MAX];
	uint32_t	all_dest = 0xFFFFFFFF;
	uint32_t	dest;

	fd_rt = fopen(RT_TABLE, "r");
	if ( fd_rt == NULL ) {
		return 2;
	}

	fgets(line_buf, LINE_MAX, fd_rt);	/* remove the header line */
	while ( fgets(line_buf, LINE_MAX, fd_rt) != NULL ) {
		sscanf(line_buf, 
			"%*s\t%08x\t%*s\t%*s\t%*s\t%*s\t%*s\t%*s\t%*s\t%*s\t%*s%*s",
			&dest);
		all_dest &= dest;
	}

	fclose(fd_rt);
	
	if ( all_dest != 0 )			/* not all network are reachable */
		return 1;
	return 0;
}
