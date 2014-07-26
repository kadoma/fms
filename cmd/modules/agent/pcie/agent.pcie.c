/*
 * agent.pcie.c
 *
 *  Created on: Jan 10, 2011
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for pcie module
 */

#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>

/**
 *	get pcie device number
 *
 *	@result:
 *		Domain:Bus:Slot
 */
static int
fmd_get_pcid(fmd_t *pfmd, char *pcid, uint64_t rscid)
{
	topo_pci_t *ppci;
	struct list_head *pos;
	fmd_topo_t *ptopo = &pfmd->fmd_topo;

	list_for_each(pos, &ptopo->list_pci) {
		ppci = list_entry(pos, topo_pci_t, list);

		if (ppci->pci_id == rscid) {
			sprintf(pcid, "%04d:%02d:%02d", ppci->pci_chassis,
				ppci->pci_hostbridge, ppci->pci_slot);
			return 0;
		}
	}

	return (-1);
}


/**
 *	fmd_pci_walk_slot
 *		find the pcie device which need to be hotpluged.
 *	@result:
 *		slot number
 */
static int
fmd_pci_walk_slot(char *pcid)
{
	char dir[] = "/sys/bus/pci/slots";
	char file1[64];
	char file2[64];
	char buf[256];
	struct dirent *dp;
	DIR *dirp;
	FILE *fp = NULL;

	memset(file1, 0, 64 * sizeof (char));
	memset(file2, 0, 64 * sizeof (char));
	memset(buf, 0, 256 * sizeof (char));

	if ((dirp = opendir(dir)) == NULL)
		return (-1); /* failed to open directory; just skip it */

	while ((dp = readdir(dirp)) != NULL) {
		if (dp->d_name[0] == '.')
			continue; /* skip "." and ".." */

		sprintf(file1, "/sys/bus/pci/slots/%s/power", dp->d_name);
		if (access(file1, 0) != 0)
			return (-1);

		sprintf(file2, "/sys/bus/pci/slots/%s/address", dp->d_name);
		if ((fp = fopen(file2, "r+")) == NULL) {
			perror("Pcie Agent: fopen\n");
			return (-1);
		}

		if (fgets(buf, 256, fp) == NULL) {
			perror("Pcie Agent: fgets\n");
			fclose(fp);
			return (-1);
		}

		if (strstr(buf, pcid) != NULL) {
			fclose(fp);
			return atoi(dp->d_name);
		} else {
			memset(file1, 0, 64 * sizeof (char));
			memset(file2, 0, 64 * sizeof (char));
			memset(buf, 0, 256 * sizeof (char));
			fclose(fp);
		}
	}

	return (-1);
}


/**
 *	Offline pcie device
 *
 *	@result:
 *		#echo 0 > /sys/bus/pci/slot/slot#/power
 */
int
pcie_offline(char *file)
{
	int fd = 0;
	if ((fd = open(file, O_RDWR)) == -1) {
		perror("Pcie Agent");
		return (-1);
	}

	write(fd, "0", 1);
	close(fd);

	return 0;
}


/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
pcie_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;
	int ret = 0;
	int slot = -1;

	/* offline */
	if ((strstr(e->ev_class, "fault.io.pcie.fail-warning")) != NULL) {
		char pcid[32];
		char buf[64];
		memset(pcid, 0, 32 * sizeof(char));
		memset(buf, 0, 64 * sizeof(char));

		if ((ret = fmd_get_pcid(pfmd, pcid, e->ev_rscid)) == 0) {
			if ((slot = fmd_pci_walk_slot(pcid)) < 0)
				return fmd_create_listevent(e, LIST_LOG);

			sprintf(buf, "/sys/bus/pci/slot/%d/power", slot);
			if ((ret = pcie_offline(buf)) == 0)
				return fmd_create_listevent(e, LIST_ISOLATED_SUCCESS);
			else
				return fmd_create_listevent(e, LIST_ISOLATED_FAILED);
		} else	/* pcie device not support hotplug, so just log */
			return fmd_create_listevent(e, LIST_LOG);
	} else { /* warning */
//		warning();
		/* ereport.io.pcie.fail-warning */
		return fmd_create_listevent(e, LIST_LOG);
	}

	// never reach
	return fmd_create_listevent(e, LIST_LOG);
}


static agent_modops_t pcie_mops = {
	.evt_handle = pcie_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&pcie_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
