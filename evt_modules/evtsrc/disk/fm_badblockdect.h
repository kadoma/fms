#ifndef __FM_BADBLOCKDECT_H
#define __FM_BADBLOCKDECT_H

#ifndef LINE_MAX
#define LINE_MAX 	1024
#endif

int disk_badblocks_check(const char *path);
#endif