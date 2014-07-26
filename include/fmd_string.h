/*
 * fmd_string.h
 *
 *  Created on: Feb 20, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	fmd_string.h
 */

#ifndef FMD_STRING_H_
#define FMD_STRING_H_

#include <string.h>
#include <assert.h>
#include <malloc.h>

/**
 * trim
 *
 * @param
 * @return
 */
char *
trim(char *string)
{
	char *fstr = string, *rstr, ch;
	char *pnew;
	int len = strlen(string);

	while(isspace(*fstr))
		fstr++;

	rstr = string + len - 1;
	while(isspace(*rstr))
		rstr--;

	len = rstr - fstr + 1;
	pnew = (char *)malloc(len);
	assert(pnew != NULL);
	strncpy(pnew, fstr, len);

	return pnew;
}


/**
 * split
 *
 * @param
 * @return
 */
char **
split(char *string, char ch, int *size)
{
	char *ptr = NULL, *str = string;
	char **result;
	int count = 0, ite;

	while((ptr = strchr(str, ch)) != NULL) {
		str = ptr + 1;
		count++;
	}

	result = (char **)malloc	\
			(++count * sizeof(char *));
	assert(result != NULL);
	*size = count;

	str = string;
	ite = 0;
	do {
		result[ite++] = str;
		ptr = strchr(str, ch);

		if(ptr != NULL) {
			*ptr = '\0';
			str = ptr + 1;
		}
	}while(ite < count);

	return result;
}

#endif /* FMD_STRING_H_ */
