/*
 * Copyright (C) inspur Inc.  <http://www.inspur.com>
 * FileName:	sysfs.c
 * Author:		Inspur OS Team
 *				changxch@inspur.com
 * Date:		2015-08-04
 * Description:	
 *				operation sysfs function. 
 *
 */

#define _GNU_SOURCE 1
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>

#include "sysfs.h"
#include "logging.h"
#include "cpumem.h"
#include "cmea_base.h"

char *
read_field(char *base, char *name)
{
	char *fn, *val;
	int n, fd;
	struct stat st;
	char *s;
	char *buf = NULL;

	asprintf(&fn, "%s/%s", base, name);
	fd = open(fn, O_RDONLY);
	if (fstat(fd, &st) < 0)
		goto bad;		
	buf = xcalloc(1, st.st_size);
	xfree(fn);
	if (fd < 0) 
		goto bad;
	n = read(fd, buf, st.st_size);
	close(fd);
	
	if (n < 0)
		goto  bad;
	val = xcalloc(1, n);
	memcpy(val, buf, n);
	xfree(buf);
	s = strchr(val, '\n');
	if (s)
		*s = 0;
	
	return  val;

bad:
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR, 
		"Cannot read sysfs field %s/%s", 
		base, name);
	return strdup("");
}

unsigned 
read_field_num(char *base, char *name)
{
	unsigned num;
	char *val = read_field(base, name);
	int n = sscanf(val, "%u", &num);
	
	free(val);
	if (n != 1) { 
		wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR,
			"Cannot parse number in sysfs field %s/%s", 
			base, name);
		return 0;
	}
	
	return num;
}

unsigned 
read_field_map(char *base, char *name, struct map *map)
{
	char *val = read_field(base, name);

	for (; map->name; map++) {
		if (!strcmp(val, map->name))
			break;
	}
	free(val);
	if (map->name)
		return map->value;
	
	wr_log(CMEA_LOG_DOMAIN, WR_LOG_ERROR,
		"sysfs field %s/%s has unknown string value `%s'", 
		base, name, val);
	return -1;
}

int 
sysfs_write(const char *name, const char *fmt, ...)
{
	int e;
	int n;
	char *buf;
	va_list ap;
	int fd = open(name, O_WRONLY);
	if (fd < 0)
		return -1;
	va_start(ap, fmt);
	n = vasprintf(&buf, fmt, ap);
	va_end(ap);
	n = write(fd, buf, n);
	e = errno;
	close(fd);
	free(buf);
	errno = e;
	return n;
}

int 
sysfs_available(const char *name, int flags)
{
	return access(name, flags) == 0;
}
