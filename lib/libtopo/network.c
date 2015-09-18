/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    network.c
 * Author:      Inspur OS Team 
                wang.leibj@inspur.com
 * Date:        2015-08-15
 * Description: get network infomation function
 *
 ************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <syslog.h>
#include <dirent.h>

#include <fmd_topo.h>
#include <fmd_errno.h>

/**
 * fmd_topo_walk_net
 *
 * @param
 * @return
 */
static int
fmd_topo_walk_net(const char *dir, char *file, fmd_topo_t *ptopo)
{
//printf("net work dir = %s \n",dir);
	char filename[50], *fptr;
	char devicepath[100];
	char *delims = ":.";
	char *token = NULL;
	uint64_t item[50], *ptr;
	DIR *dirp;
	struct dirent *dp;
	struct list_head *pos = NULL;
	topo_pci_t *ppci = NULL;

	memset(filename, 0, 50 * sizeof (char));
	memset(devicepath, 0, 100 * sizeof (char));
	memset(item, 0, 50 * sizeof (uint64_t));

	sprintf(devicepath, "%s/%s/device/driver", dir, file);

	if ((dirp = opendir(devicepath)) == NULL)
		return OPENDIR_FAILED; /* failed to open directory; just skip it */

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue; /* skip "." and ".." */
		if (strchr(dp->d_name, ':') != NULL) {
			strcpy(filename, dp->d_name);

			fptr = filename;
			ptr = item;
			while ((token = strsep(&fptr, delims)) != NULL)
				*ptr++ = (uint64_t)strtol(token, NULL, 16);

			break;
		}
	}

	list_for_each(pos, &ptopo->list_pci) {
		ppci = list_entry(pos, topo_pci_t, list);

		if ((item[0] == ppci->pci_chassis)
		 && (item[1] == ppci->pci_hostbridge)
		 && (item[2] == ppci->pci_slot)
		 && (item[3] == ppci->pci_func))
			ppci->pci_name = file;
//printf("network file = %s \n",file);
	}

	(void) closedir(dirp);

	return FMD_SUCCESS;
}


/**
 * fmd_topo_net
 *
 * @param
 * @return
 */
int
fmd_topo_net(const char *dir, fmd_topo_t *ptopo)
{
	struct dirent *dp;
	char *p;
	DIR *dirp;

	if ((dirp = opendir(dir)) == NULL)
		return OPENDIR_FAILED; /* failed to open directory; just skip it */

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue; /* skip "." and ".." */
		if (dp->d_name[0] == 'e' || dp->d_name[0] == 'w') {	/* eth* / wlan* */
			p = dp->d_name;

			fmd_topo_walk_net(dir, p, ptopo);
		}
	}
	(void) closedir(dirp);

	return FMD_SUCCESS;
}
