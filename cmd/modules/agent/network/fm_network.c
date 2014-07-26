#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ether.h>

#include "fm_network.h"
/*
 * get_if_info
 *  get the interface's infomation. Such as
 *  MAC addr, IP addr
 *
 * Input
 *  interface	- name of interface
 *
 * Output
 *  s_mac	- MAC addr in string
 *  s_ip	- ip addr in string
 *
 * Return val
 *  0		- if success
 *  non-0	- if failed
 */
int get_if_info(char *ifrname, unsigned char *s_mac, unsigned char *s_ip)
{
	int 		fd;
	struct ifreq	req;
	char		*dev_ip;
	char		*dev_mac;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( fd < 0 )
		return -1;
	strcpy(req.ifr_name, ifrname);

	/* get MAC addr */
	if ( ioctl(fd, SIOCGIFHWADDR, &req) != 0 )
		return -1;
	dev_mac = ether_ntoa( (const struct ether_addr*)req.ifr_hwaddr.sa_data );
	if ( dev_mac == NULL ){
		printf("Couldn't retrieve device MAC address\n");
		return -1;
	}
	strcpy(s_mac, dev_mac);

	/* get ip addr */
	if ( ioctl(fd, SIOCGIFADDR, &req) != 0 )
		return -1;
	dev_ip = inet_ntoa( ((struct sockaddr_in*)(&req.ifr_addr))->sin_addr );
	if ( dev_ip  == NULL ){
		printf("Couldn't retrieve device IP address\n");
		return -1;
	}
	strcpy(s_ip, dev_ip);

	close(fd);
	return 0;
}

