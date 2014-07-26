#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

/*
 * dns_check
 *  check if dns configure correct use "dig"
 *
 * Return value
 *  0		- if DNS configuration is OK
 *  1		- if DNS configuration has error
 *  other	- if some error happen
 */
int dns_check(void)
{
	char 	cmd[] = "dig www.google.com > /dev/null";
	int	ret;

	ret = system(cmd);
	if ( WEXITSTATUS(ret) )	/* error */
		return 1;

	return 0;
}
