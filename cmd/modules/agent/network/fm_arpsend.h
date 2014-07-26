#ifndef __FM_ARPSEND_H
#define __FM_ARPSEND_H

#include <netinet/if_ether.h>
#include <net/if_arp.h>

struct arp_pkt {
	struct 	ethhdr ether_hdr;
	u_short	htype;			/* hardware type (must be ARPHRD_ETHER) */
	u_short	ptype;			/* protocol type (must be ETH_P_IP) */
	u_char	hlen;			/* hardware address length (must be 6) */
	u_char	plen;			/* protocol address length (must be 4) */
	u_short	operation;		/* ARP opcode */
	u_char	sHaddr[6];		/* sender's hardware address */
	u_char	sInaddr[4];		/* sender's ip address */
	u_char	tHaddr[6];		/* target's hardware address */
	u_char	tInaddr[4];		/* target's ip address */
	u_char	pad[18];		/* pad for min. Ethernet payload */
};

#endif
