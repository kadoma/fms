#ifndef __CMEA_SYSFS_H__
#define __CMEA_SYSFS_H__

struct map { 
	char *name;
	int value;
};

extern char *read_field(char *base, char *name);
extern unsigned read_field_num(char *base, char *name);
extern unsigned read_field_map(char *base, char *name, struct map *map);

extern int sysfs_write(const char *name, const char *format, ...)
		__attribute__((format(printf,2,3)));
extern int sysfs_available(const char *name, int flags);

#endif
