/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    storage.c
 * Author:      Inspur OS Team 
                wang.leibj@inspur.com
 * Date:        2015-08-17
 * Description: get storage infomation function
 *
 ************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <syslog.h>

#include <fmd_topo.h>
#include <fmd_errno.h>

/**
 * fmd_scan_pci_storage
 *
 * @param
 * @return
 */
static int
fmd_scan_pci_storage(topo_pci_t *ppci,fmd_topo_t *ptopo)
{
	struct dirent *dp;
	const char *p;
	DIR *dirp;
	//int ret = 0;
	char dir[100];

	memset(dir, 0, 100 * sizeof (char));
	sprintf(dir, "/sys/bus/pci/devices/%04x:%02x:%02x.%x",
		ppci->pci_chassis, ppci->pci_hostbridge,
		ppci->pci_slot, ppci->pci_func);

	if ((dirp = opendir(dir)) == NULL)
		return OPENDIR_FAILED; /* failed to open directory; just skip it */

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue; /* skip "." and ".." */
		p = dp->d_name;
/* /sys/bus/pci/devices/xxxx:xx:xx.x/hostsx */
		if (strncmp(p, "host", 4) == 0) {	
			struct dirent *dp1;
			const char *p1;
			DIR *dirp1;
			char dir1[100];
			memset(dir1, 0, 100 * sizeof (char));
			sprintf(dir1, "%s/%s", dir, p);
			if ((dirp1 = opendir(dir1)) == NULL)
				return OPENDIR_FAILED; /* failed to open directory; just skip it */
			while ((dp1 = readdir(dirp1)) != NULL) {
				if (dp1->d_name[0] == '.')
					continue; /* skip "." and ".." */

				p1 = dp1->d_name;
/* /sys/bus/pci/devices/xxxx:xx:xx.x/hostsx/targetx:x:x */
				if (strncmp(p1, "target", 6) == 0) {
					struct dirent *dp2;
					char *p2;
					char tmp[20];
					DIR *dirp2;
					char dir2[100];
					memset(tmp, 0, 20* sizeof(char));
					memset(dir2, 0, 100 * sizeof (char));
					sprintf(dir2, "%s/%s", dir1, p1);

					if ((dirp2 = opendir(dir2)) == NULL)
						return OPENDIR_FAILED; /* failed to open directory; just skip it */
					while ((dp2 = readdir(dirp2)) != NULL) {
						if (dp2->d_name[0] == '.')
							continue; /* skip "." and ".." */

 					topo_storage_t *pstr = (topo_storage_t *)malloc(sizeof (topo_storage_t));
        				assert(pstr != NULL);
        				memset(pstr, 0, sizeof (topo_storage_t));

        				pstr->storage_system = ppci->pci_system;
        				pstr->storage_chassis =  ppci->pci_chassis;
        				pstr->storage_board = ppci->pci_board;
        				pstr->storage_hostbridge =  ppci->pci_hostbridge;
        				pstr->storage_slot = ppci->pci_slot;
       					pstr->storage_func = ppci->pci_func;
        				pstr->storage_topoclass = ppci->pci_topoclass;
        				pstr->dev_icon = ppci->dev_icon;
						p2 = dp2->d_name;
						sprintf(tmp,"%s",p2);
/* /sys/bus/pci/devices/xxxx:xx:xx.x/hostsX/targetX:X:X/X:X:X:X */
						if (strchr(p2, ':') != NULL) {
							char *token = NULL;
							char *delims = {":"};
							uint8_t up[4], *uptr;
							memset(up, 0, 4 * sizeof (uint8_t));
							uptr = up;
							while ((token = strsep(&p2, delims)) != NULL)
								*uptr++ = (uint8_t)strtol(token, NULL, 16);

							pstr->storage_host = up[0];	/* host */
							pstr->storage_channel = up[1];	/* channel */
							pstr->storage_target = up[2];	/* target */
							pstr->storage_lun = up[3];	/* lun */

							struct dirent *dp3;
							char *p3;
							DIR *dirp3;
							char dir3[100];
							memset(dir3, 0, 100 * sizeof (char));
							//p2 = dp2->d_name;
							sprintf(dir3, "%s/%s", dir2, tmp);
							if ((dirp3 = opendir(dir3)) == NULL)
								return OPENDIR_FAILED;

							while ((dp3 = readdir(dirp3)) != NULL) {
								if (dp3->d_name[0] == '.')
									continue; /* skip "." and ".." */

								p3 = dp3->d_name;
/* /sys/bus/pci/devices/xxxx:xx:xx.x/hostsX/targetX:X:X/X:X:X:X/block:sdx */
								if (strncmp(p3, "block", 5) == 0) {
									if (strchr(p3, ':') != NULL) {
										pstr->storage_name = strdup(&p3[6]);
										list_add(&pstr->list,&ptopo->list_storage);
										/*strcpy(pstr->storage_name, &p3[6]);*/
									} else {
										struct dirent *dp4;
										char *p4;
										DIR *dirp4;
										char dir4[100];
										memset(dir4, 0, 100 * sizeof (char));
										sprintf(dir4, "%s/%s", dir3, p3);
										if ((dirp4 = opendir(dir4)) == NULL)
											return OPENDIR_FAILED;

										while ((dp4 = readdir(dirp4)) != NULL) {
											if (dp4->d_name[0] == '.')
												continue; /* skip "." */

											p4 = dp4->d_name;
											pstr->storage_name = strdup(p4);
											list_add(&pstr->list,&ptopo->list_storage);
											/*strcpy(pstr->storage_name, p4);*/
										}
										closedir(dirp4);
									}
										
								}
								
							}
							closedir(dirp3);
						}
					}
					(void) closedir(dirp2);
				}
			}
			(void) closedir(dirp1);
		}/* end if 'host' */
	}
	(void) closedir(dirp);

	return FMD_SUCCESS;
}

/**
 * fmd_topo_walk_storage
 *
 * @param
 * @return
 */
int
fmd_topo_walk_storage(topo_pci_t *ppci, fmd_topo_t *ptopo)
{
	int ret = 0;
	ret = fmd_scan_pci_storage(ppci,ptopo);
	return ret;
}

