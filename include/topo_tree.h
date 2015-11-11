
#ifndef _TOPO_TREE_H
#define _TOPO_TREE_H 1

#include <pthread.h>

#include "list.h"

typedef struct topo_node {
	char *tn_name;			/* Node name */
	int tn_value;			/* Node value */
	int tn_state;			/* Node state (see below) */
	int tn_fflags;			/* Node exist flags */
	struct topo_node *tn_parent;	/* Node parent */
	struct topo_node *tn_child;	/* Node child */
	struct topo_node *tn_next;	/* Node next */
	int tn_refs;			/* Node reference count */
} tnode_t;

/* cpu thread */
typedef struct topo_thread {
	tnode_t *tnode;
	struct list_head list;
} thread_t;

/* cpu core */
typedef struct topo_core {
	tnode_t *tnode;
	thread_t *ht;
	struct list_head list;
	struct list_head ht_head;	/* thread list head */
} core_t;

#if 0
/* memory rank */
typedef struct topo_rank {
	tnode_t *tnode;
	struct list_head list;
} rank_t;
#endif

/* memory dimm */
typedef struct topo_dimm{
    uint64_t size;
	tnode_t *tnode;
//	rank_t *rank;
	struct list_head list;
}dimm_t;

/* memory controller */
typedef struct topo_memcontroller{
	tnode_t *tnode;
	dimm_t *dimm;
	struct list_head list;
	struct list_head dimm_head;	/* dimm list head */
}memcontroller_t;

/* cpu socket */
typedef struct topo_chip{
	tnode_t *tnode;
	core_t *core;
	memcontroller_t *mctl;
	struct list_head list;
	struct list_head core_head;	/* core list head */
	struct list_head mctl_head;	/* memcontroller list head */
}chip_t;

struct topo_hostbridge;

/* pci func */
typedef struct topo_func{
	tnode_t *tnode;
	struct topo_hostbridge *hb;	/* secondary bus */
	struct list_head list;
	struct list_head hb_head;
}func_t;

/* pci slot */
typedef struct topo_slot{
	tnode_t *tnode;
	func_t *func;
	struct list_head list;
	struct list_head func_head;	/* func list head */
}slot_t;

/* pci bus */
typedef struct topo_hostbridge {
	tnode_t *tnode;
	slot_t *slot;
	struct list_head list;
	struct list_head slot_head;	/* slot list head */
} hostbridge_t;

/* motherboard */
typedef struct topo_motherboard {
	tnode_t *tnode;
	chip_t *chip;
	hostbridge_t *hb;
	struct list_head chip_head;	/* chip list head */
	struct list_head hb_head;	/* hostbridge list head */
} motherboard_t;

/* systemboard */
typedef struct topo_systemboard {
	tnode_t *tnode;
} systemboard_t;

/* chassis */
typedef struct topo_chassis {
	tnode_t *tnode;
	motherboard_t *mb;
	systemboard_t *sb;
	struct list_head list;
} chassis_t;

#define TOPO_NODE_INIT          0x0001
#define TOPO_NODE_ROOT          0x0002
#define TOPO_NODE_BOUND         0x0004
#define TOPO_NODE_LINKED        0x0008

typedef struct topo_tree {
	struct list_head tt_list;	/* next/prev pointers */
	struct list_head tt_root;	/* root node */
} ttree_t;

#endif  /* _TOPO_TREE_H */
