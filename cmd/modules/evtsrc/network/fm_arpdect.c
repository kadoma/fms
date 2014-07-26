#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include "fm_arpdect.h"

#define MAC_BROADCAST_ADDR	(unsigned char*) "\xff\xff\xff\xff\xff\xff"
#define MAX_TIMEOUT		2	/* max seconds to wait for reply */

typedef struct arp_pkg {
	struct ethhdr ethhdr;                   /* Ethernet header */
	u_short htype;                          /* hardware type (must be ARPHRD_ETHER) */
	u_short ptype;                          /* protocol type (must be ETH_P_IP) */
	u_char  hlen;                           /* hardware address length (must be 6) */
	u_char  plen;                           /* protocol address length (must be 4) */
	u_short operation;                      /* ARP opcode */
	u_char  sHaddr[6];                      /* sender's hardware address */
	u_char  sInaddr[4];                     /* sender's IP address */
	u_char  tHaddr[6];                      /* target's hardware address */
	u_char  tInaddr[4];                     /* target's IP address */
	u_char  pad[18];                        /* pad for min. Ethernet payload (60 bytes) */
} arp_pkg_t;

/*
 * get_if_info
 *  get the interface's infomation. Such as
 *  MAC addr, IP addr
 *
 * Input
 *  interface	- name of interface
 *
 * Output
 *  mac		- MAC addr
 *  ip		- ip addr
 *
 * Return val
 *  0		- if success
 *  non-0	- if failed
 */
int get_if_info(char *ifrname, unsigned char *mac, in_addr_t *ip)
{
	int 		fd;
	struct ifreq	req;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( fd < 0 )
		return -1;
	strcpy(req.ifr_name, ifrname);

	/* get MAC addr */
	if ( ioctl(fd, SIOCGIFHWADDR, &req) != 0 )
		return -1;
	memcpy(mac, req.ifr_hwaddr.sa_data, ETH_ALEN);

	/* get ip addr */
	if ( ioctl(fd, SIOCGIFADDR, &req) != 0 )
		return -1;
	memcpy(ip, &((struct sockaddr_in *)(&req.ifr_addr))->sin_addr, 4/*FIXME*/);

	close(fd);
	return 0;
}

/*
 * check_ip
 *  check if our ip own by other host 
 *  though sending arp package
 *
 * Input
 *  interface	- interface used to send pkg
 *  dst_ip	- IP addr which will be check
 *
 * Return Value
 *  0		- if IP addr not been used
 *  1		- if IP addr has been used
 *  -1		- error
 */
int check_ip(char *interface/*, in_addr_t dst_ip*/)
{
	int		exist = 0;	/* is it dst_ip already exist? */
	int		fd;
	int		optval =  1;
	unsigned char	s_macaddr[6];	/* our MAC addr */
	in_addr_t	s_ipaddr;	/* our IP addr */
	arp_pkg_t	arp_req;
	struct sockaddr	sa;

	fd_set		read_fds;	
	struct timeval	tm;
	time_t		prev_time;
	int		timeout = MAX_TIMEOUT;

	/* acquire own addr */
	if ( get_if_info(interface, s_macaddr, &s_ipaddr) != 0 ) {
		fprintf(stderr, "Couldn't acquire MAC & IP addr\n");
		return -1;
	}

	/* open socket */
	fd = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));
	if ( fd < 0 )			/* open socket failed */
		return -1;
	if ( setsockopt(fd, SOL_SOCKET, 
			SO_BROADCAST, 
			&optval, 
			sizeof(optval)) < 0 )
		return -1;

	/* fill the packet header */
	memset(&arp_req, 0, sizeof(arp_pkg_t));
	memcpy(arp_req.ethhdr.h_dest, MAC_BROADCAST_ADDR, 6);
	memcpy(arp_req.ethhdr.h_source, s_macaddr, 6);
	arp_req.ethhdr.h_proto	= htons(ETH_P_ARP);		/* protocol type */
	arp_req.htype	= htons(ARPHRD_ETHER);			/* hardware type */
	arp_req.ptype	= htons(ETH_P_IP);				/* protocol type (ARP message) */
	arp_req.hlen	= 6;							/* hardware addr length */
	arp_req.plen	= 4;							/* protocol address length */
	arp_req.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	memcpy(arp_req.sHaddr, s_macaddr, 6);			/* source MAC addr */
	memcpy(arp_req.tInaddr, &s_ipaddr, 4);
	*(in_addr_t *)arp_req.sInaddr = 0;				/* set to 0 for IP conflict dect */
	//*(in_addr_t *)arp_req.sInaddr = s_ipaddr;		/* source IP addr */
	//*(in_addr_t *)arp_req.tInaddr = s_ipaddr;		/* target IP addr */

	/* send the request */
	memset(&sa, 0, sizeof(sa));
	strcpy(sa.sa_data, interface);
	if ( sendto(fd, &arp_req, sizeof(arp_req), 0, &sa, sizeof(sa)) < 0 ) {
		fprintf(stderr, "NETWORK: send arp request failed\n");
		return -1;
	}

	/* wait for reply */
	tm.tv_usec = 0;
	time(&prev_time);			/* get current time */
	while (timeout > 0) {
		tm.tv_sec = timeout;

		/* wait for sock ready */
		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);
		if ( select( fd+1, &read_fds, NULL, NULL, &tm) < 0 ) {
			fprintf(stderr, "NETWORK: error occur while wait for reply\n");
			return -1;
		}

		/* receive reply */
		if ( recv(fd, &arp_req, sizeof(arp_req), 0)  < 0 ) {
			fprintf(stderr, "NETWORK: recv arp request failed\n");
			return -1;
		}

		/* check if the right reply we expected */
		if ( arp_req.operation == htons(ARPOP_REPLY) &&		/* it's a arp reply */
			bcmp(arp_req.tHaddr, s_macaddr, 6) == 0 &&	/* and it's to us */
			/* TODO I think we should check if the sender's mac addr 
			   different from us */
			*((in_addr_t*)arp_req.sInaddr) == s_ipaddr) {	/* and it's from right place */
			exist = 1;
			break;
		}
		
		timeout = timeout - (time(NULL) - prev_time);
		time(&prev_time);
	}

	close(fd);

	return exist;
}

#if 0
int main(int argc, char *argv[])
{
	in_addr_t	dst_ip;
	unsigned char	*mac;
	in_addr_t	ip;
	int		chkip;

	if ( argc < 2 ) {
		printf("too few argument\n");
		exit(-1);
	}
	dst_ip = inet_addr(argv[1]);
	mac = (char*)malloc(16);
	get_if_info("eth0", mac, &ip);

	chkip = check_ip("eth0", dst_ip);
	if ( chkip < 0 ){
		printf("check_ip error\n");
		return -1;
	}else if ( chkip == 0 ) 
		printf("IP free\n");
	else
		printf("IP has been used\n");

	return 0;
}
#endif
