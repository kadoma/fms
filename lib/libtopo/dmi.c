/*
 * dmi.c
 *
 *  Created on: Dec 13, 2010
 *      Author: Inspur OS Team
 *  
 *  Description:
 *  	dmi.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/mman.h> 
#include <linux/limits.h>

#include <fmd.h>
#include <fmd_errno.h>
#include <fmd_list.h>
#include <fmd_topo.h>

struct dmi_header {
	uint8_t type;
	uint8_t length;
	uint16_t handle;
};

/* dmi data */
typedef struct dmi_data {
	struct list_head processor_list;
	struct list_head array_list;
	struct list_head device_list;
	struct list_head array_addr_list;
	struct list_head device_addr_list;
} dmi_data_t;

/* socket */
typedef struct dmi_processor {
	uint64_t handle;
	uint64_t processor_id;
	char socket_designation[32];
	struct list_head list;
} dmi_processor_t;

/* memory array */
typedef struct dmi_mem_array {
	uint64_t handle;
	uint64_t devices;
	struct list_head list;
} dmi_mem_array_t;

/* memory device */
typedef struct dmi_mem_device {
	uint64_t handle;
	uint64_t array_handle;
	uint64_t size;
	char device_locator[32];
	char bank_locator[32];
	struct list_head list;
} dmi_mem_device_t;

/* array mapped address */
typedef struct dmi_array_map_addr {
	uint64_t handle;
	uint64_t array_handle;
	uint64_t start;
	uint64_t end;
	uint64_t extended_start;
	uint64_t extended_end;
	struct list_head list;
} dmi_array_map_addr_t;

/* device mapped address */
typedef struct dmi_device_map_addr {
	uint64_t handle;
	uint64_t device_handle;
	uint64_t array_addr_handle;
	uint64_t start;
	uint64_t end;
	uint64_t extended_start;
	uint64_t extended_end;
	struct list_head list;
} dmi_device_map_addr_t;


/**
 * get_efi_systab_smbios
 *
 * @param
 * @return
 */
static long
get_efi_systab_smbios(void)
{
	long result = 0;
	FILE *fp = NULL;
	char buffer[100];
	char buf[100];

	memset(buffer, 0, 100 * sizeof (char));
	memset(buf, 0, 100 * sizeof (char));

	if ((fp = fopen("/sys/firmware/efi/systab", "r")) == NULL) {
		printf("failed to open efi systab for SMBIOS\n");
		return (0);
	}

	while (fgets(buffer, sizeof(buffer), fp)) {
		if ((buffer[0] == '\n') || (buffer[0] == '\0'))
			continue;	/* skip blank line and '#' line*/
		if (strncmp(buffer, "SMBIOS", 6) == 0) {
			strcpy(buf, &buffer[7]);
			result = strtol(buf, NULL, 16);

			return result;
		}
	}
	fclose(fp);

	return result;
}


/**
 * dmi_data_free
 *
 * @param
 * @return
 */
static void
dmi_data_free(dmi_data_t *pdmi)
{
	/* free processor list */
	struct list_head *pos = NULL;
	dmi_processor_t *dp = NULL;
	list_for_each(pos, &pdmi->processor_list) {
		dp = list_entry(pos, dmi_processor_t, list);
		free(dp);
	}

	/* free memory array list */
	struct list_head *dpos = NULL;
	dmi_mem_array_t *dma = NULL;
	list_for_each(dpos, &pdmi->array_list) {
		dma = list_entry(dpos, dmi_mem_array_t, list);
		free(dma);
	}

	/* free memory device list */
	struct list_head *ddpos = NULL;
	dmi_mem_device_t *dmd = NULL;
	list_for_each(ddpos, &pdmi->device_list) {
		dmd = list_entry(ddpos, dmi_mem_device_t, list);
		free(dmd);
	}

	/* free memory array addr list */
	struct list_head *apos = NULL;
	dmi_array_map_addr_t *dama = NULL;
	list_for_each(apos, &pdmi->array_addr_list) {
		dama = list_entry(apos, dmi_array_map_addr_t, list);
		free(dama);
	}

	/* free memory device addr list */
	struct list_head *dapos = NULL;
	dmi_device_map_addr_t *ddma = NULL;
	list_for_each(dapos, &pdmi->device_addr_list) {
		ddma = list_entry(dapos, dmi_device_map_addr_t, list);
		free(ddma);
	}
}


/**
 * dmi_table
 *
 * @param
 * @return
 */
static dmi_data_t *
dmi_table(int fd, uint32_t base, int len, int num)
{
	unsigned char *buf = (unsigned char *) malloc(len);
	struct dmi_header *dm;
	dmi_data_t *pdmi;
	uint8_t *data;
	int i = 0;
	uint32_t mmoffset = 0;
	void *mmp = NULL;

	/* malloc */
	pdmi = (dmi_data_t *)malloc(sizeof (dmi_data_t));
	assert(pdmi != NULL);
	memset(pdmi, 0, sizeof (dmi_data_t));

	INIT_LIST_HEAD(&pdmi->processor_list);
	INIT_LIST_HEAD(&pdmi->array_list);
	INIT_LIST_HEAD(&pdmi->device_list);
	INIT_LIST_HEAD(&pdmi->array_addr_list);
	INIT_LIST_HEAD(&pdmi->device_addr_list);

	if (len == 0)
		/* no data */
		return NULL;

	if (buf == NULL)
		/* memory exhausted */
		return NULL;

	mmoffset = base % getpagesize();

	mmp = mmap(0, mmoffset + len, PROT_READ, MAP_SHARED, fd, base - mmoffset);
	if (mmp == MAP_FAILED) {
		free(buf);
		return NULL;
	}
	memcpy(buf, (uint8_t *) mmp + mmoffset, len);
	munmap(mmp, mmoffset + len);

	data = buf;
	while (data + sizeof(struct dmi_header) <= (uint8_t *) buf + len) {
		int type = -1;
		dm = (struct dmi_header *) data;

		/*
		 * we won't read beyond allocated memory
		 */
		if (data + dm->length > (uint8_t *) buf + len)
		/* incomplete structure, abort decoding */
			break;

		type = dm->type;
		if (type == 4) {		// Processor
			dmi_processor_t *dmip = NULL;
			/* malloc */
			dmip = (dmi_processor_t *)malloc(sizeof (dmi_processor_t));
			assert(dmip != NULL);
			memset(dmip, 0, sizeof (dmi_processor_t));

			dmip->handle = data[2];
			dmip->processor_id = data[8];
			strcpy(dmip->socket_designation, &data[4]);
			list_add(&dmip->list, &pdmi->processor_list);
		} else if (type == 16) {	// Physical Memory Array
			if (data[5] == 0x03) {		/* System Memory */
				dmi_mem_array_t *dma = NULL;
				/* malloc */
				dma = (dmi_mem_array_t *)malloc(sizeof (dmi_mem_array_t));
				assert(dma != NULL);
				memset(dma, 0, sizeof (dmi_mem_array_t));

				dma->handle = data[2];
				dma->devices = data[13];
				list_add(&dma->list, &pdmi->array_list);
			}
		} else if (type == 17) {	// Memory Device
			uint32_t u;
			dmi_mem_device_t *dmd = NULL;
			/* malloc */
			dmd = (dmi_mem_device_t *)malloc(sizeof (dmi_mem_device_t));
			assert(dmd != NULL);
			memset(dmd, 0, sizeof (dmi_mem_device_t));

			dmd->handle = data[2];
			dmd->array_handle = (data[5] << 8 | data[4]);

			u = data[13] << 8 | data[12];
			if (u != 0xffff)
				dmd->size = (1024ULL * (u & 0x7fff) * ((u & 0x8000) ? 1 : 1024ULL));
			
			strcpy(dmd->device_locator, &data[16]);
			strcpy(dmd->bank_locator, &data[17]);
			list_add(&dmd->list, &pdmi->device_list);
		} else if (type == 19) {	// Memory Array Mapped Address
			dmi_array_map_addr_t *dama = NULL;
			/* malloc */
			dama = (dmi_array_map_addr_t *)malloc(sizeof (dmi_array_map_addr_t));
			assert(dama != NULL);
			memset(dama, 0, sizeof (dmi_array_map_addr_t));

			dama->handle = data[2];
			dama->array_handle = (data[0x0D] << 8 | data[0x0C]);
			dama->start = ((data[4] | data[5] << 8) | (data[6] | data[7] << 8) << 16);
			dama->end = ((data[8] | data[9] << 8) | (data[10] | data[11] << 8) << 16);
			if (dama->end - dama->start < 512) { /* memory range is smaller thant 512KB */
				/* consider that values were expressed in megagytes */
				dama->start *= 1024;
				dama->end *= 1024;
			}
		
			dama->extended_start = 0;
			dama->extended_end = 0;
			list_add(&dama->list, &pdmi->array_addr_list);
		} else if (type == 20) {	// Memory Device Mapped Address
			dmi_device_map_addr_t *ddma = NULL;
			/* malloc */
			ddma = (dmi_device_map_addr_t *)malloc(sizeof (dmi_device_map_addr_t));
			assert(ddma != NULL);
			memset(ddma, 0, sizeof (dmi_device_map_addr_t));

			ddma->handle = data[2];
			ddma->device_handle = (data[0x0D] << 8 | data[0x0C]);
			ddma->array_addr_handle = data[0x0E];
			ddma->start = ((data[4] | data[5] << 8) | (data[6] | data[7] << 8) << 16);
			ddma->end = ((data[8] | data[9] << 8) | (data[10] | data[11] << 8) << 16);
			if (ddma->end - ddma->start < 512) { /* memory range is smaller than 512KB */
				/* consider that values were expressed in megagytes */
				ddma->start *= 1024;
				ddma->end *= 1024;
			}

			ddma->extended_start = 0;
			ddma->extended_end = 0;
			list_add(&ddma->list, &pdmi->device_addr_list);
		}
		data += dm->length;
		while (*data || data[1])
			data++;
		data += 2;
		i++;
	} //while
	free(buf);

	return pdmi;
}


/**
 * comp
 *
 * @param
 * @return
 */
static int
comp(const void *a,const void *b)
{
	return *(uint64_t *)a - *(uint64_t *)b;
}


/**
 * fmd_topo_walk_mem
 *
 * @param
 * @return
 * TODO: socket to memory controller
 *	 all begin 0 to max
 */
static void
fmd_topo_walk_mem(dmi_data_t *pdmi, fmd_topo_t *ptopo)
{
	struct list_head *pos = NULL;
	topo_cpu_t *pcpu = NULL;
	int max_chassis = 0;
	int num_socket[1024];
	int num_dimm[1024];
	int num_d = 0, num_a = 0, num = 0;
	int i = 0, j = 0, k = 0;

	list_for_each(pos, &ptopo->list_cpu) {
		pcpu = list_entry(pos, topo_cpu_t, list);
		/* default: begin 0 */
		if (pcpu->cpu_chassis > max_chassis)
			max_chassis = pcpu->cpu_chassis;
	}

	dmi_mem_device_t *dmd = NULL;
	dmi_mem_array_t *dma = NULL;
	struct list_head *ppos = NULL;
	struct list_head *apos = NULL;
	list_for_each(ppos, &pdmi->device_list) {
		dmd = list_entry(ppos, dmi_mem_device_t, list);
		num_d++;	/* Sum of devices */
	}

	list_for_each(apos, &pdmi->array_list) {
		dma = list_entry(apos, dmi_mem_array_t, list);
		num_a++;	/* Sum of arrays */
	} 

	uint64_t device_handle[num_d], *p1;
	uint64_t array_handle[num_a], *p2;
	dmi_mem_device_t *pdmd = NULL;
	dmi_mem_array_t *pdma = NULL;
	struct list_head *dpos = NULL;
	struct list_head *dppos = NULL;

	list_for_each(dpos, &pdmi->device_list) {
		pdmd = list_entry(dpos, dmi_mem_device_t, list);
		p1 = device_handle;
		*p1++ = pdmd->handle;
	}
	list_for_each(dppos, &pdmi->array_list) {
		pdma = list_entry(dppos, dmi_mem_array_t, list);
		p2 = array_handle;
		*p2++ = pdma->handle;
		/* Attention: lvalue(p2) required as
		   increment operand must be correctable. */
	}
		
	qsort(device_handle, num_d, sizeof(uint64_t), comp);
	qsort(array_handle, num_a, sizeof(uint64_t), comp);

	/* chassis */
	for (i = 0; i < max_chassis + 1; i++) {
		struct list_head *pos1 = NULL;
		topo_cpu_t *pcpu1 = NULL;
		int max_socket = 0;
		
		list_for_each(pos1, &ptopo->list_cpu) {
			pcpu1 = list_entry(pos1, topo_cpu_t, list);
			/* default: begin 0 */
			if ((pcpu1->cpu_chassis == i) && (pcpu1->cpu_socket > max_socket))
				max_socket = pcpu1->cpu_socket;
		}
		num_socket[i] = max_socket;

		/* socket */
		for (j = 0; j < num_socket[i] + 1; j++) {
			struct list_head *pos2 = NULL;
			dmi_mem_array_t *ppdma = NULL;
			int max_dimm = 0;
			uint64_t handle = array_handle[i * (max_socket + 1) + j];

			list_for_each(pos2, &pdmi->array_list) {
				ppdma = list_entry(pos2, dmi_mem_array_t, list);
				if (handle == ppdma->handle)
					max_dimm = ppdma->devices;
			}
			num_dimm[j] = max_dimm;

			/* dimm */
			for (k = 0; k < num_dimm[j]; k++) {
				topo_mem_t *pmem = NULL;
				struct list_head *pos3 = NULL;
				dmi_device_map_addr_t *pddma = NULL;
				uint64_t handle2 = device_handle[num++];
				/* malloc */
				pmem = (topo_mem_t *)malloc(sizeof (topo_mem_t));
				assert(pmem != NULL);
				memset(pmem, 0, sizeof (topo_mem_t));

				pmem->mem_system = 0;
				pmem->mem_chassis = i;
				pmem->mem_board = 0;
				pmem->mem_socket = j;
				pmem->mem_controller = 0;
				pmem->mem_dimm = k;
				pmem->mem_topoclass = TOPO_MEMORY;

				list_for_each(pos3, &pdmi->device_addr_list) {
					pddma = list_entry(pos3, dmi_device_map_addr_t, list);
					if (handle2 == pddma->device_handle) {
						pmem->start = pddma->start;
						pmem->end = pddma->end;
					}
				}
				list_add(&pmem->list, &ptopo->list_mem);
			}
		}
	}
}


/**
 * fmd_topo_dmi
 *
 * @param
 * @return
 */
int
fmd_topo_dmi(fmd_topo_t *ptopo)
{
	unsigned char buf[20];
	int fd = 0;
	long fp = 0;
	uint32_t mmoffset = 0;
	void *mmp = NULL;
	bool efi = true;
	dmi_data_t *pdmi = NULL;

	fd = open("/dev/mem", O_RDONLY);
	if (fd == -1)
		return (-1);

	fp = get_efi_systab_smbios();
	if (fp <= 0) {
		efi = false;
		fp = 0xE0000L;		/* default value for non-EFI capable platforms */
	}

	fp -= 16;
	while (efi || (fp < 0xFFFFF)) {
		fp += 16;
		mmoffset = fp % getpagesize();
		mmp = mmap(0, mmoffset + 0x20, PROT_READ, MAP_SHARED, fd, fp - mmoffset);
		memset(buf, 0, sizeof(buf));
		if (mmp != MAP_FAILED) {
			memcpy(buf, (uint8_t *) mmp + mmoffset, sizeof(buf));
			munmap(mmp, mmoffset + 0x20);
		}

		if (mmp == MAP_FAILED) {
			close(fd);
			return (-1);
		} else if (memcmp(buf, "_DMI_", 5) == 0) {
			uint16_t num = buf[13] << 8 | buf[12];
			uint16_t len = buf[7] << 8 | buf[6];
			uint32_t base = buf[11] << 24 | buf[10] << 16 | buf[9] << 8 | buf[8];

			if ((pdmi = dmi_table(fd, base, len, num)) == NULL) {
				close(fd);
				return (-1);
			}
			fmd_topo_walk_mem(pdmi, ptopo);

			if (efi)
				break;	/* we don't need to search the memory for EFI systems */
		}
	}
	close(fd);
//	dmi_data_free(pdmi);

	return FMD_SUCCESS;
}

