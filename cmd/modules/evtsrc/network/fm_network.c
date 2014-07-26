/*
 * file:	fm_network.c
 * describe:	This file is used in arpdect, for acquire 
 *		some interface infomation such as IP address
 *		and hardware address.
 *		May be I should merge it into arpdect. TODO
 */
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ether.h>

