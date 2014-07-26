#ifndef __FM_ARPING_H
#define __FM_ARPING_H

#include <linux/if_ether.h>

struct ip_conflict_msg {
        char            c_ifr_name[LINE_MAX];   /* conflict in which infterface? */
        unsigned char   c_haddr[ETH_ALEN];      /* hardware address which have the same IP addr with us */
        int             c_serverity;            /* can perform defend? */
};

int check_ip(char *interface);/* check if ip conflict*/
int get_if_info(char *ifrname, unsigned char *mac, in_addr_t *ip);

#endif
