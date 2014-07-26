/*
 * agent.network.c
 *
 *  Created on: Jan 10, 2011
 *      Author: Inspur OS Team
 *
 *  Descriptions:
 *  	Agent for network module
 */

#include <stdio.h>
#include <fmd_agent.h>
#include <fmd_event.h>
#include <fmd_api.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "fm_network.h"
#include "fm_arpsend.h"

#define MAC_BROADCAST_ADDR	(unsigned char*) "\xff\xff\xff\xff\xff\xff"

/*
 * arp_announcement
 *  send an arp announcement packet, defend own IP addr.
 *  An annoucement is a ARP request containing the 
 *  sender's IP address in the target IP field, target 
 *  hardware addresses set to zero.
 *
 * Input
 *  interface	- name of network interface 
 *  s_ipaddr	- source IP address (conflict address)
 *  s_hwaddr	- source hardware address
 *
 * Return value
 *  0		- if success
 *  non-0	- if failed
 */
static int arp_announcement(char *interface, in_addr_t s_ipaddr,
		unsigned char* s_hwaddr)
{
	int 		fd;
	int		optval = 1;
	struct arp_pkt	arp_ann;	/* ARP announcement use for defend IP */
	struct sockaddr	sa;

	/* open socket */
	fd = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));
	if ( fd < 0 )
		return 1;
	if ( setsockopt(fd, SOL_SOCKET,
			SO_BROADCAST,
			&optval,
			sizeof(optval)) < 0)
		return 1;

	/* fill packet header */
	memset(&arp_ann, 0, sizeof(arp_ann));
	memcpy(arp_ann.ether_hdr.h_dest, MAC_BROADCAST_ADDR, 6);
	memcpy(arp_ann.ether_hdr.h_source, s_hwaddr, 6);
	arp_ann.ether_hdr.h_proto = htons(ETH_P_ARP);
	arp_ann.htype	= htons(ARPHRD_ETHER);
	arp_ann.ptype	= htons(ETH_P_IP);
	arp_ann.hlen	= 6;
	arp_ann.plen	= 4;
	arp_ann.operation = htons(ARPOP_REQUEST);
	memcpy(arp_ann.sHaddr, s_hwaddr, 6);
	*(in_addr_t*)arp_ann.sInaddr = s_ipaddr;
	*(in_addr_t*)arp_ann.tInaddr = s_ipaddr;

	/* send announcement */
	memset(&sa, 0, sizeof(sa));
	strcpy(sa.sa_data, interface);
	if ( sendto(fd, &arp_ann, sizeof(arp_ann), 0, &sa, sizeof(sa)) < 0){
		return 1;
	}

	return 0;
}

/**
 *	handle the fault.* event
 *
 *	@result:
 *		list.* event
 */
fmd_event_t *
network_handle_event(fmd_t *pfmd, fmd_event_t *e)
{
	fmd_debug;
	char	*interface;		/* interface name */	
	unsigned char	s_macaddr[6];	
	in_addr_t s_ipaddr;
	fmd_event_t	*elog;

	elog = fmd_create_listevent(e, LIST_LOG);
	/* warning */
	if (strstr(e->ev_class, "unknown") != NULL)
		return elog;
	else if ( strstr(e->ev_class, "ipconflict") != NULL) {	/* ipconflict? */
		/* acquire interface info */
		interface = topo_pci_by_pcid(pfmd, e->ev_rscid);
		if (get_if_info(interface, s_macaddr, (unsigned char *)&s_ipaddr) !=0) {
			fprintf(stderr, "Couldn't acquire MAC & IP addr\n");
			return fmd_create_listevent(e, LIST_REPAIRED_FAILED);
		}
		
		/* send announcement */
		if ( arp_announcement(interface, 
					s_ipaddr, s_macaddr) ) {/* send aouncement failed */
			return fmd_create_listevent(e, LIST_REPAIRED_FAILED);
		}
		else
			return fmd_create_listevent(e, LIST_REPAIRED_SUCCESS);

	}
	else {	/* other */
//		warning();
		return elog;
	}

	// never reach
	return elog;
}


static agent_modops_t network_mops = {
	.evt_handle = network_handle_event,
};


fmd_module_t *
fmd_init(const char *path, fmd_t *pfmd)
{
	fmd_debug;
	return (fmd_module_t *)agent_init(&network_mops, path, pfmd);
}


void
fmd_fini(fmd_module_t *mp)
{
}
