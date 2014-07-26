/*
 * =========================================================
 *
 *       Filename:  fm_netdect.c
 *
 *    Description:  get network status
 *
 *        Version:  1.0
 *        Created:  09/20/2010 01:17:11 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: Inspur OS Team
 *
 * =========================================================
 */

#include	<stdio.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<syslog.h>
#include	<string.h>
#include 	<sys/types.h>
#include 	<sys/ioctl.h>
#include 	<sys/stat.h>
#include 	<stdio.h>
#include 	<string.h>
#include 	<errno.h>
#include 	<net/if.h>
#include 	<linux/sockios.h>
#include 	<linux/ethtool.h>
#include 	"fm_netdect.h"

static void get_info(struct net_dev *nd)
{
	int	err;
	int 	fd;
	char	*p;
	struct ethtool_drvinfo 	drvinfo;
	struct ethtool_value 	edata;
	struct ifreq 		ifr;		/* ioctl use */

	/* Open socket for ioctl */
	if ( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ){
		syslog(LOG_USER|LOG_INFO, "Cannot get control socket\n");
		return;
	}

	/* Setup our control structures. */
	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, nd->name);

	drvinfo.cmd = ETHTOOL_GDRVINFO;		/* Get bus_info */
	ifr.ifr_data = (caddr_t)&drvinfo;
	err = ioctl(fd, SIOCETHTOOL, &ifr);
	if (err < 0 || (p=strstr(drvinfo.bus_info, "N/A")) != NULL ) {
		nd->need_update = 0;
		close(fd);
		return;
	}
	strcpy(nd->bus_info, drvinfo.bus_info);

	edata.cmd = ETHTOOL_GLINK;		/*  get link state info */
	ifr.ifr_data = (caddr_t)&edata;
	err = ioctl(fd, SIOCETHTOOL, &ifr);	/* if edata.data == 0, link down 
	   					   if edata.data == 1, link up
						   else link error */
	if (err == 0) {
		nd->link_state = !!edata.data;
	} else {
		nd->link_state = NETLINK_ERR;
	}
	nd->need_update = 1;

	close(fd);
	return;
}

static inline void _update_statics(struct net_dev *ori, struct net_dev nd)
{
	ori->prev_tx_err = ori->tx_err;
	ori->prev_rx_err = ori->rx_err;
	ori->prev_link_state = ori->link_state;
	
	/* link_state changed, but bus-info is the same, 2 things may be 
	   happened:
	   1. the same card was DOWN and UP now;
	   2. change to other card, it's a new card, 
	   reset all error count to 0 */
	if ( (ori->link_state = nd.link_state) != ori->prev_link_state )
	{
		ori->rx_err = 0;
		ori->rx_err_count = 0;
		ori->prev_rx_err = 0;
		ori->tx_err = 0;
		ori->tx_err_count = 0;
		ori->prev_tx_err = 0;
        	ori->need_report = 1;
	}else if ( (ori->tx_err = nd.tx_err) != ori->prev_tx_err ||
        	(ori->rx_err = nd.rx_err) != ori->prev_rx_err )
        	ori->need_report = 1;
	else
		ori->need_report = 0;
}

/*
 * check nd to be reported ot not
 * if devp[i] alread exist the nd->bus_info, update devp[i] and 
 * decide to be report or not;
 * if not in devp list(the first time to get the device info or 
 * a new device added/removed), add to the list and report it.
 */
static void update_static(struct net_dev **devp, struct net_dev nd)
{
	int	i;
	struct	net_dev *ndp;

	for (i=0; devp[i] != NULL && i < LINE_MAX; ++i){
		if ( devp[i]->bus_info == NULL )
			continue;
		if (!strcmp(devp[i]->bus_info, nd.bus_info)) { /* the same bus*/
			_update_statics(devp[i], nd);
			return;
		}
	}

	/*  a new device, add to list and mark to report */
	ndp = (struct net_dev*)malloc(sizeof(*ndp));
	memset(ndp, 0, sizeof(*ndp));
	*ndp = nd;
	devp[i] = ndp;
	ndp->need_report = 1;

	return;
}

void get_ethinfo(char *linebuf, struct net_dev **devp)
{
	int i = -1, j;
	struct net_dev nd;	/* store the err info */
	
	memset(&nd, 0, sizeof(nd));

	/*  skip the blank space, and copy the name first */
	while (linebuf[++i] == ' ' );
		for (j=0; linebuf[i+j] != ':'; j++)
			nd.name[j] = linebuf[i+j];

	linebuf[i+j]	= ' ';	/* make sscanf happy */
	nd.name[j]	= '\0';

	get_info(&nd);		/*  get the info by the dev name */
	if (nd.need_update) {	/*  if need_update, get the statics */
		sscanf(linebuf, "%*s %*d %*d %d %*d %*d"
			    "%*d %*d %*d %*d %*d"
			    "%d %*d %*d %*d %*d %*d\n",
	  			&(nd.tx_err), &(nd.rx_err));
		update_static(devp, nd);
	}
	/*printf("%s %d %d\n", nd->name, nd->tx_err, nd->rx_err);*/
}

