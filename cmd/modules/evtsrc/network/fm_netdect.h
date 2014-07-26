/*
 * =============================================================
 *
 *       Filename:  fm_netdect.h
 *
 *    Description:  for fmd netdevice state detect
 *    				copy from ethtool source file
 *
 *        Version:  1.0
 *        Created:  09/21/2010 10:48:56 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: Inspur OS Team
 *
 * =============================================================
 */

#ifndef SIOCETHTOOL
#define SIOCETHTOOL     0x8946
#endif

#ifndef LINE_MAX
#define LINE_MAX 	1024
#endif

#define NETLEN 		64	

#define NETLINK_DOWN	0
#define NETLINK_UP	1
#define NETLINK_ERR	2

struct net_dev {
	char	name[NETLEN];		/* Name of interface */
	char	bus_info[NETLEN];	/* Name of bus */
	int	tx_err;			
	int	prev_tx_err;
	int	tx_err_count;

	int	rx_err;
	int	prev_rx_err;
	int	rx_err_count;

	int	link_state;
	int	prev_link_state;
	int	need_update;	/* Do not need update if it's a virtual interface */
	int	need_report;
};

void get_ethinfo(char *linebuf, struct net_dev **devp);
