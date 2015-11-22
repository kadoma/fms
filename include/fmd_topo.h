
#ifndef FMD_TOPO_H_
#define FMD_TOPO_H_ 1

#include <stdint.h>
#include <list.h>
#include <topo_tree.h>

#define DIR_SYS_CPU  "/sys/devices/system/node"
#define DIR_SYS_PCI  "/sys/bus/pci/devices"
#define DIR_SYS_NET  "/sys/class/net"
#define DIR_SYS_DISK "/sys/block"

typedef struct fmd_topo {
	struct list_head list_cpu;
	struct list_head list_mem;
	struct list_head list_pci;
	struct list_head list_pci_host;
	struct list_head list_pci_bridge;
	struct list_head list_storage;
	ttree_t *tp_root;
} fmd_topo_t;

typedef enum topo_class {
	TOPO_SYSTEM,
	TOPO_BRIDGE,
	TOPO_MEMORY,
	TOPO_PROCESSOR,
	TOPO_STORAGE,
	TOPO_DISK,
	TOPO_BUS,
	TOPO_NETWORK,
	TOPO_DISPLAY,
	TOPO_INPUT,
	TOPO_POWER,
	TOPO_GENERIC
} topoclass_t;

/**
 * cpu
 *
 */
typedef struct topo_cpu {
	/* processor id */
	int processor;

	/*
	 * traverse flag for setting up thread-id
	 *
	 * flag can be used for extension, too
	 * i.g topology tree informations can store here.
	 */
	int flag;

	/* cpuid */
	union {
		struct {
			uint32_t system:8;
			uint32_t chassis:8;
			uint32_t board:8;
			uint32_t socket:8;
			uint32_t core:8;
			uint32_t thread:8;
			uint32_t reserved:11;
			uint32_t topoclass:5;
		} _struct_topo_cpu;
		uint64_t cpuid;
	} _union_topo_cpu;

	/* cpu list */
	struct list_head list;
} topo_cpu_t;

#define cpu_id _union_topo_cpu.cpuid
#define cpu_system _union_topo_cpu._struct_topo_cpu.system
#define cpu_chassis _union_topo_cpu._struct_topo_cpu.chassis
#define cpu_board _union_topo_cpu._struct_topo_cpu.board
#define cpu_socket _union_topo_cpu._struct_topo_cpu.socket
#define cpu_core _union_topo_cpu._struct_topo_cpu.core
#define cpu_thread _union_topo_cpu._struct_topo_cpu.thread
#define cpu_topoclass _union_topo_cpu._struct_topo_cpu.topoclass


/**
 * memory
 *
 */
typedef struct topo_mem {
	/* memory range */
	uint64_t start;
	uint64_t end;

	/*
	 * traverse flag for setting up thread-id
	 *
	 * flag can be used for extension, too
	 * i.g topology tree informations can store here.
	 */
	int flag;

	/* memid */
	union {
		struct {
			uint32_t system:8;
			uint32_t chassis:8;
			uint32_t board:8;
			uint32_t socket:8;
			uint32_t controller:8;
			uint32_t dimm:8;
			uint32_t reserved:11;
			uint32_t topoclass:5;
		} _struct_topo_mem;
		uint64_t memid;
	} _union_topo_mem;

	/* mem list */
	struct list_head list;
} topo_mem_t;

#define mem_id _union_topo_mem.memid
#define mem_system _union_topo_mem._struct_topo_mem.system
#define mem_chassis _union_topo_mem._struct_topo_mem.chassis
#define mem_board _union_topo_mem._struct_topo_mem.board
#define mem_socket _union_topo_mem._struct_topo_mem.socket
#define mem_controller _union_topo_mem._struct_topo_mem.controller
#define mem_dimm _union_topo_mem._struct_topo_mem.dimm
#define mem_topoclass _union_topo_mem._struct_topo_mem.topoclass


/**
 * pci
 *
 */
typedef struct topo_pci {
	/* pcid */
	union {
		struct {
			uint32_t system:8;
			uint32_t chassis:8;		/* domain */
			uint32_t board:8;		/* motherboard: 0 */
			uint32_t hostbridge:8;		/* bus */
			uint32_t slot:8;		/* slot */
			uint32_t func:4;		/* func */
			uint32_t subdomain:2;		/* primary: always 0000 ? */
			uint32_t subbus:8;		/* secondary */
			uint32_t subslot:3;		/* sub-slot */
			uint32_t subfunc:2;		/* sub-func */
			uint32_t topoclass:5;
		} _struct_topo_pci;
		uint64_t pcid;
	} _union_topo_pci;

	/* name */
	char *pci_name;

	/* device icon */
	char *dev_icon;

	/* list */
	struct list_head list;
} topo_pci_t;

#define pci_id _union_topo_pci.pcid
#define pci_system _union_topo_pci._struct_topo_pci.system
#define pci_chassis _union_topo_pci._struct_topo_pci.chassis
#define pci_board _union_topo_pci._struct_topo_pci.board
#define pci_hostbridge _union_topo_pci._struct_topo_pci.hostbridge
#define pci_slot _union_topo_pci._struct_topo_pci.slot
#define pci_func _union_topo_pci._struct_topo_pci.func
#define pci_subdomain _union_topo_pci._struct_topo_pci.subdomain
#define pci_subbus _union_topo_pci._struct_topo_pci.subbus
#define pci_subslot _union_topo_pci._struct_topo_pci.subslot
#define pci_subfunc _union_topo_pci._struct_topo_pci.subfunc
#define pci_topoclass _union_topo_pci._struct_topo_pci.topoclass


/**
 * storage
 *
 */
typedef struct topo_storage {
	/* storageid */
	union {
		struct {
			uint32_t system:8;
			uint32_t chassis:8;		/* domain */
			uint32_t board:8;		/* motherboard: 0 */
			uint32_t hostbridge:8;		/* bus */
			uint32_t slot:8;		/* slot */
			uint32_t func:8;		/* func */
			uint32_t host:8;		/* host id */
			uint32_t channel:8;		/* channel id */
			uint32_t target:8;		/* target id */
			uint32_t lun:8;			/* lun id */
			uint32_t reserved:3;
			uint32_t topoclass:5;
		} _struct_topo_storage;
		uint64_t storageid;
	} _union_topo_storage;

	/* name */
	char *storage_name;	/* eg. sda */

	/* device icon */
	char *dev_icon;

	/* list */
	struct list_head list;
} topo_storage_t;

#define storage_id _union_topo_storage.storageid
#define storage_system _union_topo_storage._struct_topo_storage.system
#define storage_chassis _union_topo_storage._struct_topo_storage.chassis
#define storage_board _union_topo_storage._struct_topo_storage.board
#define storage_hostbridge _union_topo_storage._struct_topo_storage.hostbridge
#define storage_slot _union_topo_storage._struct_topo_storage.slot
#define storage_func _union_topo_storage._struct_topo_storage.func
#define storage_host _union_topo_storage._struct_topo_storage.host
#define storage_channel _union_topo_storage._struct_topo_storage.channel
#define storage_target _union_topo_storage._struct_topo_storage.target
#define storage_lun _union_topo_storage._struct_topo_storage.lun
#define storage_topoclass _union_topo_storage._struct_topo_storage.topoclass

/****************************************************************
 * Helper Routines
 ***************************************************************/



#if 0
uint64_t topo_get_cpuid(fmd_t *, int);
uint64_t topo_get_pcid(fmd_t *, char *);
uint64_t topo_get_memid(fmd_t *, uint64_t);
uint64_t topo_get_storageid(fmd_t *, char *);

int topo_cpu_by_cpuid(fmd_t *, uint64_t);
uint64_t topo_start_by_memid(fmd_t *, uint64_t);
uint64_t topo_end_by_memid(fmd_t *, uint64_t);
char *topo_pci_by_pcid(fmd_t *, uint64_t);
char *topo_storage_by_storageid(fmd_t *, uint64_t);
#endif
#endif /* FMD_TOPO_H_ */
