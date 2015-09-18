
#ifndef __DMI_H__
#define __DMI_H___ 1

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

//static long get_efi_systab_smbios(void);
//static dmi_data_t *dmi_table(int fd, uint32_t base, int len, int num);
//static int comp(const void *a,const void *b);
//static void fmd_topo_walk_mem(dmi_data_t *pdmi, fmd_topo_t *ptopo);
int fmd_topo_dmi(fmd_topo_t *ptopo);
#endif
