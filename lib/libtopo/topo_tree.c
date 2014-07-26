/*
 * topo_tree.c
 *
 *  Created on: Sep 19, 2010
 *      Author: Inspur OS Team
 *
 *  Description:
 *      TOPO Tree
 */

#include <stdio.h>
#include <assert.h>
#include <fmd.h>
#include <fmd_topo.h>
#include <fmd_list.h>
#include <topo_tree.h>

static tnode_t *
topo_tnode_alloc(void)
{
	tnode_t *node = malloc(sizeof (tnode_t));
	assert(node != NULL);
	memset(node, 0, sizeof (tnode_t));
	node->tn_fflags = 1;
	return node;
}

static chassis_t *
topo_chassis_alloc(void)
{
	chassis_t *ccp = (chassis_t *)malloc(sizeof (chassis_t));
	assert(ccp != NULL);
	memset(ccp, 0, sizeof (chassis_t));

	ccp->tnode = topo_tnode_alloc();
	ccp->tnode->tn_name = "chassis";
	ccp->tnode->tn_state = TOPO_NODE_LINKED;
	ccp->mb = NULL;
	ccp->sb = NULL;
	INIT_LIST_HEAD(&ccp->list);
	return ccp;
}

static motherboard_t *
topo_motherboard_alloc(void)
{
	motherboard_t *mb = (motherboard_t *)malloc(sizeof (motherboard_t));
	assert(mb != NULL);
	memset(mb, 0, sizeof (motherboard_t));

	mb->tnode = topo_tnode_alloc();
	mb->tnode->tn_name = "motherboard";
	mb->tnode->tn_state = TOPO_NODE_LINKED;
	mb->chip = NULL;
	mb->hb = NULL;
	INIT_LIST_HEAD(&mb->chip_head);
	INIT_LIST_HEAD(&mb->hb_head);
	return mb;
}

static chip_t *
topo_chip_alloc(void)
{
	chip_t *chp = (chip_t *)malloc(sizeof (chip_t));
	assert(chp != NULL);
	memset(chp, 0, sizeof (chip_t));

	chp->tnode = topo_tnode_alloc();
	chp->tnode->tn_name = "chip";
	chp->tnode->tn_state = TOPO_NODE_LINKED;
	chp->core = NULL;
	chp->mctl = NULL;
	INIT_LIST_HEAD(&chp->list);
	INIT_LIST_HEAD(&chp->core_head);
	INIT_LIST_HEAD(&chp->mctl_head);
	return chp;
}

static core_t *
topo_core_alloc(void)
{
	core_t *cop = (core_t *)malloc(sizeof (core_t));
	assert(cop != NULL);
	memset(cop, 0, sizeof (core_t));

	cop->tnode = topo_tnode_alloc();
	cop->tnode->tn_name = "core";
	cop->tnode->tn_state = TOPO_NODE_LINKED;
	cop->ht = NULL;
	INIT_LIST_HEAD(&cop->list);
	INIT_LIST_HEAD(&cop->ht_head);
	return cop;
}

static memcontroller_t *
topo_mctl_alloc(void)
{
	memcontroller_t *mctp = (memcontroller_t *)malloc(sizeof (memcontroller_t));
	assert(mctp != NULL);
	memset(mctp, 0, sizeof (memcontroller_t));

	mctp->tnode = topo_tnode_alloc();
	mctp->tnode->tn_name = "memory-controller";
	mctp->tnode->tn_state = TOPO_NODE_LINKED;
	mctp->dimm = NULL;
	INIT_LIST_HEAD(&mctp->list);
	INIT_LIST_HEAD(&mctp->dimm_head);
	return mctp;
}

static dimm_t *
topo_dimm_alloc(void)
{
	dimm_t *dip = (dimm_t *)malloc(sizeof (dimm_t));
	assert(dip != NULL);
	memset(dip, 0, sizeof (dimm_t));

	dip->tnode = topo_tnode_alloc();
	dip->tnode->tn_name = "dimm";
	dip->tnode->tn_state = TOPO_NODE_LINKED;
	INIT_LIST_HEAD(&dip->list);
	return dip;
}

static thread_t *
topo_thread_alloc(void)
{
	thread_t *ht = (thread_t *)malloc(sizeof (thread_t));
	assert(ht != NULL);
	memset(ht, 0, sizeof (thread_t));

	ht->tnode = topo_tnode_alloc();
	ht->tnode->tn_name = "thread";
	ht->tnode->tn_state = TOPO_NODE_LINKED;
	INIT_LIST_HEAD(&ht->list);
	return ht;
}

static hostbridge_t *
topo_hostbridge_alloc(void)
{
	hostbridge_t *hb = (hostbridge_t *)malloc(sizeof (hostbridge_t));
	assert(hb != NULL);
	memset(hb, 0, sizeof (hostbridge_t));

	hb->tnode = topo_tnode_alloc();
	hb->tnode->tn_name = "hostbridge";
	hb->tnode->tn_state = TOPO_NODE_LINKED;
	hb->slot = NULL;
	INIT_LIST_HEAD(&hb->list);
	INIT_LIST_HEAD(&hb->slot_head);
	return hb;
}

static slot_t *
topo_slot_alloc(void)
{
	slot_t *slp = (slot_t *)malloc(sizeof (slot_t));
	assert(slp != NULL);
	memset(slp, 0, sizeof (slot_t));

	slp->tnode = topo_tnode_alloc();
	slp->tnode->tn_name = "slot";
	slp->tnode->tn_state = TOPO_NODE_LINKED;
	slp->func = NULL;
	INIT_LIST_HEAD(&slp->list);
	INIT_LIST_HEAD(&slp->func_head);
	return slp;
}

static func_t *
topo_func_alloc(void)
{
	func_t *fcp = (func_t *)malloc(sizeof (func_t));
	assert(fcp != NULL);
	memset(fcp, 0, sizeof (func_t));

	fcp->tnode = topo_tnode_alloc();
	fcp->tnode->tn_name = "func";
	fcp->tnode->tn_state = TOPO_NODE_LINKED;
	fcp->hb = NULL;
	INIT_LIST_HEAD(&fcp->list);
	INIT_LIST_HEAD(&fcp->hb_head);
	return fcp;
}

static void
print_topo_thread(core_t *cp)
{
	struct list_head *pos = NULL;
	thread_t *tp = NULL;
	/* traverse thread */
	list_for_each(pos, &cp->ht_head) {
		tp = list_entry(pos, thread_t, list);

		printf("|	|       |       |       |-- thread%d\n", tp->tnode->tn_value);
	}
}

static void
print_topo_dimm(memcontroller_t *mcp)
{
	struct list_head *pos = NULL;
	dimm_t *dp = NULL;
	/* traverse dimm */
	list_for_each(pos, &mcp->dimm_head) {
		dp = list_entry(pos, dimm_t, list);

		printf("|	|       |       |       |-- dimm%d\n", dp->tnode->tn_value);
	}
}

static void
print_topo_core(chip_t *chp)
{
	struct list_head *pos = NULL;
	struct list_head *ppos = NULL;
	core_t *cp = NULL;
	memcontroller_t *mcp = NULL;
	/* traverse core */
	list_for_each(pos, &chp->core_head) {
		cp = list_entry(pos, core_t, list);

		printf("|	|       |       |-- core%d\n", cp->tnode->tn_value);
		print_topo_thread(cp);
	}

	if (chp->mctl != NULL) {
		/* traverse mem-controller */
		list_for_each(ppos, &chp->mctl_head) {
			mcp = list_entry(ppos, memcontroller_t, list);

			printf("|       |       |       |-- memory-controller%d\n", mcp->tnode->tn_value);
			print_topo_dimm(mcp);
		}
	}
}

static void print_topo_slot(hostbridge_t *hbp, int sub);

static void
print_topo_func(slot_t *slp, int sub)
{
	struct list_head *pos = NULL;
	struct list_head *ppos = NULL;
	func_t *fp = NULL;
	hostbridge_t *hbp = NULL;
	/* traverse func */
	list_for_each(pos, &slp->func_head) {
		fp = list_entry(pos, func_t, list);

		if (sub == 0)
			printf("|	|       |       |       |-- func%d\n", fp->tnode->tn_value);
		else if (sub == 1)
			printf("|	|       |       |       |       |       |       |-- func%d\n", fp->tnode->tn_value);

		if (fp->hb == NULL)
			continue;
		else {
			/* traverse hostbridge */
			list_for_each(ppos, &fp->hb_head) {
				hbp = list_entry(ppos, hostbridge_t, list);

				printf("|	|       |       |       |       |-- hostbridge%d\n", hbp->tnode->tn_value);
				print_topo_slot(hbp, 1);
			}
		}
	}
}

static void
print_topo_slot(hostbridge_t *hbp, int sub)
{
	struct list_head *pos = NULL;
	slot_t *slp = NULL;
	/* traverse slot */
	list_for_each(pos, &hbp->slot_head) {
		slp = list_entry(pos, slot_t, list);

		if (sub == 0) {
			printf("|	|       |       |-- slot%d\n", slp->tnode->tn_value);
			print_topo_func(slp, 0);
		} else if (sub == 1) {
			printf("|	|       |       |       |       |       |-- slot%d\n", slp->tnode->tn_value);
			print_topo_func(slp, 1);
		}
	}
}

static void
print_topo_motherboard(motherboard_t *mb)
{
	struct list_head *pos = NULL;
	struct list_head *ppos = NULL;
	chip_t *chp = NULL;
	hostbridge_t *hbp = NULL;
	/* traverse chip */
	list_for_each(pos, &mb->chip_head) {
		chp = list_entry(pos, chip_t, list);

		printf("|       |       |-- chip%d\n", chp->tnode->tn_value);
		print_topo_core(chp);
	}
	/* traverse hostbridge */
	list_for_each(ppos, &mb->hb_head) {
		hbp = list_entry(ppos, hostbridge_t, list);

		printf("|       |       |-- hostbridge%d\n", hbp->tnode->tn_value);
		print_topo_slot(hbp, 0);
	}
}

static void
print_topo_systemboard(systemboard_t *sb)
{
	/* FIXME: print system borad information */
	printf("|       |-- systemboard0\n");
}

static void
print_topo_chassis(chassis_t *csp)
{
	printf("|-- chassis%d\n", csp->tnode->tn_value);
	printf("|	|-- motherboard%d\n", csp->mb->tnode->tn_value);

	print_topo_motherboard(csp->mb);
	print_topo_systemboard(csp->sb);
}

void
print_topo_tree(fmd_topo_t *ptp)
{
	printf(".\n");

	struct list_head *pos = NULL;
	chassis_t *csp = NULL;
	/* traverse chassis */
	list_for_each(pos, &ptp->tp_root->tt_root) {
		csp = list_entry(pos, chassis_t, list);

		print_topo_chassis(csp);
	}
}

static int
fmd_topo_tree(fmd_topo_t *ptp)
{
	struct list_head *pos = NULL;
	struct list_head *ppos = NULL;
	struct list_head *mpos = NULL;
	topo_cpu_t *pcpu = NULL;
	topo_pci_t *ppci = NULL;
	topo_mem_t *pmem = NULL;

	/* init root list head */
	INIT_LIST_HEAD(&ptp->tp_root->tt_root);

	/* traverse cpu */ 
	list_for_each(pos, &ptp->list_cpu) {
		pcpu = list_entry(pos, topo_cpu_t, list);
		/* chassis */
		chassis_t *ccp = NULL;
		if (list_empty(&ptp->tp_root->tt_root)) {	/* chassis list head */
			ccp = topo_chassis_alloc();
			ccp->tnode->tn_value = pcpu->cpu_chassis;
			list_add(&ccp->list, &ptp->tp_root->tt_root);
		} else {
			struct list_head *apos = NULL;
			chassis_t *cp = NULL;
			list_for_each(apos, &ptp->tp_root->tt_root) {
				cp = list_entry(apos, chassis_t, list);

				if (cp->tnode->tn_value == pcpu->cpu_chassis) {
					ccp = cp;
					break;
				}
			}
		
			if (ccp == NULL) {	/* chassis not found */
				ccp = topo_chassis_alloc();
				ccp->tnode->tn_value = pcpu->cpu_chassis;
				list_add(&ccp->list, &ptp->tp_root->tt_root);
			}
		}
		/* board */
		motherboard_t *mb = NULL;
		systemboard_t *sb = NULL;
		if (ccp->mb == NULL) {
			mb = topo_motherboard_alloc();
			mb->tnode->tn_value = pcpu->cpu_board;
			ccp->mb = mb;
		} else if (ccp->sb == NULL) {
			sb = (systemboard_t *)malloc(sizeof (systemboard_t));
			assert(sb != NULL);
			memset(sb, 0, sizeof (systemboard_t));

			sb->tnode = topo_tnode_alloc();
			sb->tnode->tn_name = "systemboard";
			sb->tnode->tn_value = 1;	/* systemboard: 1 */
			sb->tnode->tn_state = TOPO_NODE_LINKED;
			ccp->sb = sb;
		}
		/* chip (socket) */
		chip_t *chp = NULL;
		if (ccp->mb->chip == NULL) {		/* chip list head */
			ccp->mb->chip = topo_chip_alloc();
			ccp->mb->chip->tnode->tn_value = pcpu->cpu_socket;
			chp = ccp->mb->chip;
			list_add(&chp->list, &ccp->mb->chip_head);
		} else {
			chip_t *chip = NULL;
			struct list_head *cpos = NULL;
			list_for_each(cpos, &ccp->mb->chip_head) {
				chip = list_entry(cpos, chip_t, list);

				if (chip->tnode->tn_value == pcpu->cpu_socket) {
					chp = chip;
					break;
				}
			}

			if (chp == NULL) {      /* chip not found */
				chp = topo_chip_alloc();
				chp->tnode->tn_value = pcpu->cpu_socket;
				list_add(&chp->list, &ccp->mb->chip_head);
			}
		}
		/* core */
		core_t *cop = NULL;
		if (chp->core == NULL) {		/* core list head */
			chp->core = topo_core_alloc();
			chp->core->tnode->tn_value = pcpu->cpu_core;
			cop = chp->core;
			list_add(&cop->list, &chp->core_head);
		} else {
			core_t *core = NULL;
			struct list_head *opos = NULL;
			list_for_each(opos, &chp->core_head) {
				core = list_entry(opos, core_t, list);

				if (core->tnode->tn_value == pcpu->cpu_core) {
					cop = core;
					break;
				}
			}

			if (cop == NULL) {      /* core not found */
				cop = topo_core_alloc();
				cop->tnode->tn_value = pcpu->cpu_core;
				list_add(&cop->list, &chp->core_head);
			}
		}
		/* thread */
		thread_t *ht = NULL;
		if (cop->ht == NULL) {			/* thread list head */
			cop->ht = topo_thread_alloc();
			cop->ht->tnode->tn_value = pcpu->cpu_thread;
			ht = cop->ht;
			list_add(&ht->list, &cop->ht_head);
		} else {
			thread_t *thread = NULL;
			struct list_head *tpos = NULL;
			list_for_each(tpos, &cop->ht_head) {
				thread = list_entry(tpos, thread_t, list);

				if (thread->tnode->tn_value == pcpu->cpu_thread) {
					ht = thread;
					break;
				}
			}

			if (ht == NULL) {	/* thread not found */
				ht = topo_thread_alloc();
				ht->tnode->tn_value = pcpu->cpu_thread;
				ht->tnode->tn_state = TOPO_NODE_BOUND;
				list_add(&ht->list, &cop->ht_head);
			}
		}
	} /* list_for_each */

	/* traverse pci */
	list_for_each(ppos, &ptp->list_pci) {
		ppci = list_entry(ppos, topo_pci_t, list);
		/* chassis */
		chassis_t *csp = NULL;
		chassis_t *pcp = NULL;
		struct list_head *papos = NULL;
		list_for_each(papos, &ptp->tp_root->tt_root) {
			pcp = list_entry(papos, chassis_t, list);

			if (pcp->tnode->tn_value == ppci->pci_chassis) {
				csp = pcp;
				break;
			}
		}

		if (csp == NULL || csp->mb == NULL) {	/* chassis/motherboard not found */
			printf("fmd topo: pci host node can't find the chassis/motherboard node\n");
			return (-1);
		}
		/* hostbridge */
		hostbridge_t *hb = NULL;
		if (csp->mb->hb == NULL) {		/* hostbridge list head */
			csp->mb->hb = topo_hostbridge_alloc();
			csp->mb->hb->tnode->tn_value = ppci->pci_hostbridge;
			hb = csp->mb->hb;
			list_add(&hb->list, &csp->mb->hb_head);
		} else {
			hostbridge_t *hbridge = NULL;
			struct list_head *hbpos = NULL;
			list_for_each(hbpos, &csp->mb->hb_head) {
				hbridge = list_entry(hbpos, hostbridge_t, list);

				if (hbridge->tnode->tn_value == ppci->pci_hostbridge) {
					hb = hbridge;
					break;
				}
			}

			if (hb == NULL) {	/* hostbridge not found */
				hb = topo_hostbridge_alloc();
				hb->tnode->tn_value = ppci->pci_hostbridge;
				list_add(&hb->list, &csp->mb->hb_head);
			}
		}
		/* slot */
		slot_t *slp = NULL;
		if (hb->slot == NULL) {			/* slot list head */
			hb->slot = topo_slot_alloc();
			hb->slot->tnode->tn_value = ppci->pci_slot;
			slp = hb->slot;
			list_add(&slp->list, &hb->slot_head);
		} else {
			slot_t *slot = NULL;
			struct list_head *slpos = NULL;
			list_for_each(slpos, &hb->slot_head) {
				slot = list_entry(slpos, slot_t, list);

				if (slot->tnode->tn_value == ppci->pci_slot) {
					slp = slot;
					break;
				}
			}

			if (slp == NULL) {	/* slot not found */
				slp = topo_slot_alloc();
				slp->tnode->tn_value = ppci->pci_slot;
				list_add(&slp->list, &hb->slot_head);
			}
		}
		/* func */
		func_t *fcp = NULL;
		if (slp->func == NULL) {		/* func list head */
			slp->func = topo_func_alloc();
			slp->func->tnode->tn_value = ppci->pci_func;
			fcp = slp->func;
			list_add(&fcp->list, &slp->func_head);
		} else {
			func_t *func = NULL;
			struct list_head *fcpos = NULL;
			list_for_each(fcpos, &slp->func_head) {
				func = list_entry(fcpos, func_t, list);

				if (func->tnode->tn_value == ppci->pci_func) {
					fcp = func;
					break;
				}
			}

			if (fcp == NULL) {	/* func not found */
				fcp = topo_func_alloc();
				fcp->tnode->tn_value = ppci->pci_func;
				list_add(&fcp->list, &slp->func_head);
			}
		}
		/* subdomain */
		if (ppci->pci_subbus == 0 && ppci->pci_subdomain == 0)
			continue;
		else {
			hostbridge_t *fhb = NULL;
			if (fcp->hb == NULL) {		/* hostbridge list head */
				fcp->hb = topo_hostbridge_alloc();
				fcp->hb->tnode->tn_value = ppci->pci_subbus;
				fhb = fcp->hb;
				list_add(&fhb->list, &fcp->hb_head);
			} else {
				hostbridge_t *fhbridge = NULL;
				struct list_head *htpos = NULL;
				list_for_each(htpos, &fcp->hb_head) {
					fhbridge = list_entry(htpos, hostbridge_t, list);

					if (fhbridge->tnode->tn_value == ppci->pci_subbus) {
						fhb = fhbridge;
						break;
					}
				}

				if (fhb == NULL) {      /* hostbridge not found */
					fhb = topo_hostbridge_alloc();
					fhb->tnode->tn_value = ppci->pci_subbus;
					list_add(&fhb->list, &fcp->hb_head);
				}
			}
			/* slot */
			slot_t *sslp = NULL;
			if (fhb->slot == NULL) {			/* slot list head */
				fhb->slot = topo_slot_alloc();
				fhb->slot->tnode->tn_value = ppci->pci_subslot;
				sslp = fhb->slot;
				list_add(&sslp->list, &fhb->slot_head);
			} else {
				slot_t *sslot = NULL;
				struct list_head *sslpos = NULL;
				list_for_each(sslpos, &fhb->slot_head) {
					sslot = list_entry(sslpos, slot_t, list);

					if (sslot->tnode->tn_value == ppci->pci_subslot) {
						sslp = sslot;
						break;
					}
				}

				if (sslp == NULL) {      /* slot not found */
					sslp = topo_slot_alloc();
					sslp->tnode->tn_value = ppci->pci_subslot;
					list_add(&sslp->list, &fhb->slot_head);
				}
			}
			/* func */
			func_t *pfcp = NULL;
			if (sslp->func == NULL) {                /* func list head */
				sslp->func = topo_func_alloc();
				sslp->func->tnode->tn_value = ppci->pci_subfunc;
				pfcp = sslp->func;
				list_add(&pfcp->list, &sslp->func_head);
			} else {
				func_t *pfunc = NULL;
				struct list_head *fupos = NULL;
				list_for_each(fupos, &sslp->func_head) {
					pfunc = list_entry(fupos, func_t, list);

					if (pfunc->tnode->tn_value == ppci->pci_subfunc) {
						pfcp = pfunc;
						break;
					}
				}

				if (pfcp == NULL) {      /* func not found */
					pfcp = topo_func_alloc();
					pfcp->tnode->tn_value = ppci->pci_subfunc;
					list_add(&pfcp->list, &sslp->func_head);
				}
			}
		}
	} /* list_for_each */

	/* traverse memory */
	list_for_each(mpos, &ptp->list_mem) {
		pmem = list_entry(mpos, topo_mem_t, list);
		/* chassis */
		chassis_t *mcsp = NULL;
		chassis_t *mpcp = NULL;
		struct list_head *pmpos = NULL;
		list_for_each(pmpos, &ptp->tp_root->tt_root) {
			mpcp = list_entry(pmpos, chassis_t, list);

			if (mpcp->tnode->tn_value == pmem->mem_chassis) {
				mcsp = mpcp;
				break;
			}
		}

		if (mcsp == NULL || mcsp->mb == NULL) {	/* chassis/motherboard not found */
			printf("fmd topo: memory controller node can't find the chassis/motherboard node\n");
			return (-1);
		}
		/* chip (socket) */
		if (mcsp->mb->chip == NULL) {		/* chip list head */
			printf("fmd topo: memory controller node can't find the chip node\n");
			return (-1);
		}
		chip_t *mchp = NULL;
		chip_t *mchip = NULL;
		struct list_head *mcpos = NULL;
		list_for_each(mcpos, &mcsp->mb->chip_head) {
			mchip = list_entry(mcpos, chip_t, list);

			if (mchip->tnode->tn_value == pmem->mem_socket) {
				mchp = mchip;
				break;
			}
		}

		if (mchp == NULL) {	/* chip not found */
			printf("fmd topo: memory controller node can't find the chip node\n");
			return (-1);
		}
		/* memory controller */
		memcontroller_t *pmctl = NULL;
		if (mchp->mctl == NULL) {		/* mem controller list head */
			mchp->mctl = topo_mctl_alloc();
			mchp->mctl->tnode->tn_value = pmem->mem_controller;
			pmctl = mchp->mctl;
			list_add(&pmctl->list, &mchp->mctl_head);
		} else {
			memcontroller_t *memctl = NULL;
			struct list_head *mepos = NULL;
			list_for_each(mepos, &mchp->mctl_head) {
				memctl = list_entry(mepos, memcontroller_t, list);

				if (memctl->tnode->tn_value == pmem->mem_controller) {
					pmctl = memctl;
					break;
				}
			}

			if (pmctl == NULL) {	/* controller not found */
				pmctl = topo_mctl_alloc();
				pmctl->tnode->tn_value = pmem->mem_controller;
				list_add(&pmctl->list, &mchp->mctl_head);
			}
		}
		/* dimm */
		dimm_t *pdm = NULL;
		if (pmctl->dimm == NULL) {		/* dimm list head */
			pmctl->dimm = topo_dimm_alloc();
			pmctl->dimm->tnode->tn_value = pmem->mem_dimm;
			pdm = pmctl->dimm;
			list_add(&pdm->list, &pmctl->dimm_head);
		} else {
			dimm_t *pdimm = NULL;
			struct list_head *dimpos = NULL;
			list_for_each(dimpos, &pmctl->dimm_head) {
				pdimm = list_entry(dimpos, dimm_t, list);

				if (pdimm->tnode->tn_value == pmem->mem_dimm) {
					pdm = pdimm;
					break;
				}
			}

			if (pdm == NULL) {    /* dimm not found */
				pdm = topo_dimm_alloc();
				pdm->tnode->tn_value = pmem->mem_dimm;
				list_add(&pdm->list, &pmctl->dimm_head);
			}
		}
	} /* list_for_each */

	return 0;
}

int
topo_tree_create(fmd_t *fmd)
{
	fmd_topo_t *ptp = &fmd->fmd_topo;
	ttree_t *tp = NULL;

	tp = (ttree_t *)malloc(sizeof (ttree_t));
	assert(tp != NULL);
	INIT_LIST_HEAD(&tp->tt_root);
	INIT_LIST_HEAD(&tp->tt_list);

	ptp->tp_root = tp;

	if (fmd_topo_tree(ptp) != 0)
		return (-1);

	return 0;
}

void
topo_tree_destroy(fmd_t *fmd)
{

}

