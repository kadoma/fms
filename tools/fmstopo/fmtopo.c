/************************************************************
 * Copyright (C) inspur Inc. <http://www.inspur.com>
 * FileName:    fmtopo.c
 * Author:      Inspur OS Team 
                wang.leibj@inspur.com
 * Date:        2015-08-11
 * Description: the topo calling interface
 *
 ************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <dlfcn.h>
#include <linux/limits.h>
#include <fmd.h>

static void
usage(void)
{
	fputs("Usage: fmtopo [-hcpt]\n"
	      "\t-h    help\n"
	      "\t-c    display cpu topology\n"
              "\t-m    display mem topology\n"
              "\t-d    display disk topology\n"
	      "\t-t    display a tree formatted topology\n"
	      , stderr);
}

/**
 * main
 *
 * @param
 * @return
 */
int
main(int argc, char *argv[])
{
	fmd_t fmd;
	char path[PATH_MAX], *error = NULL;
	int c = 0, ret = 0;
	void *handle;
	void (*fmd_topo)(fmd_t *);
	int  (*topo_tree_create)(fmd_t *);
	void (*print_cpu_topo)(fmd_topo_t *);
	//void (*print_pci_topo)(fmd_topo_t *);
	void (*print_mem_topo)(fmd_topo_t *);
	void (*print_disk_topo)(fmd_topo_t *);
	void (*print_topo_tree)(fmd_topo_t *);

	if (argc == 1) {
		usage();
		return 0;
	}

	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", BASE_DIR, "libfmd_topo.so");
	handle = dlopen(path, RTLD_LAZY);
	if (handle == NULL) {
		syslog(LOG_ERR, "dlopen");
		exit(-1);
	}

	/* fmd_topo */
	dlerror();
	fmd_topo = dlsym(handle, "_fmd_topo");
	if ((error = dlerror()) != NULL) {
	
	syslog(LOG_ERR, "dlsym");
		exit(-1);
	}
	(*fmd_topo)(&fmd);

	/* topo_tree_create */
	dlerror();
	topo_tree_create = dlsym(handle, "topo_tree_create");
	if((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}

	ret = (*topo_tree_create)(&fmd);
	if(ret < 0) {
		syslog(LOG_ERR, "topo_tree_create");
		exit(-1);
	}

	/* ready for print_cpu_topo*/
	dlerror(); 
	print_cpu_topo = dlsym(handle, "_print_cpu_topo");
	if ((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}
	#if 0
	/* ready for print_pci_topo */
	dlerror();
	print_pci_topo = dlsym(handle, "_print_pci_topo");
	if ((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}
	#endif
	/* ready for print_mem_topo */
        dlerror();
        print_mem_topo = dlsym(handle, "_print_mem_topo");
        if ((error = dlerror()) != NULL) {
                syslog(LOG_ERR, "dlsym");
                exit(-1);
        }

 	/* ready for print_disk_topo */
        dlerror();
        print_disk_topo = dlsym(handle, "_print_disk_topo");
        if ((error = dlerror()) != NULL) {
                syslog(LOG_ERR, "dlsym");
                exit(-1);
        }

	/* ready for print topo tree */
	dlerror();
	print_topo_tree = dlsym(handle, "print_topo_tree");
	if ((error = dlerror()) != NULL) {
		syslog(LOG_ERR, "dlsym");
		exit(-1);
	}

	while ((c = getopt(argc, argv, "hcpmdt")) != -1) {
		switch (c) {
		case 'h':
			usage();
			break;
		case 'c':
			/* print_cpu_topo */

			(*print_cpu_topo)(&fmd.fmd_topo);
			break;
		#if 0
		case 'p':
			/* print_pci_topo */
			(*print_pci_topo)(&fmd.fmd_topo);
			break;
		#endif
                case 'm':
                        /* print_pci_topo */

                        (*print_mem_topo)(&fmd.fmd_topo);
                        break;
                case 'd':
                        /* print_pci_topo */
                        (*print_disk_topo)(&fmd.fmd_topo);
                        break;
		case 't':
			/* tree */
			printf("----    System Topology Tree    ----\n");
			printf("\n");
			(*print_topo_tree)(&fmd.fmd_topo);
			printf("\n");
			break;
		default:
			usage();
		}
	}

	dlclose(handle);
	return 0;
}
