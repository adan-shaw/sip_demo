#ifndef __SIP_H__
#define __SIP_H__

#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>					//bzero
#include <pthread.h>
#include <sys/ioctl.h>			//ioctl 命令
#include <linux/if_ether.h>	//ethhdr 结构
#include <net/if.h>					//ifreq 结构
#include <netinet/in.h>			//in_addr 接结构
#include <linux/udp.h>			//udphdr 结构
#include <linux/tcp.h>			//tcphdr 结构
#include <stdlib.h>
#include "sip_ether.h"
#include "sip_skbuff.h"
#include "sip_arp.h"
#include "sip_ip.h"
#include "sip_icmp.h"
#include "sip_udp.h"
#include "sip_socket.h"

enum netconn_state 
{
NETCONN_NONE, 
NETCONN_WRITE, 
NETCONN_LISTEN, 
NETCONN_CONNECT, 
NETCONN_CLOSE 
};


enum netconn_evt 
{
NETCONN_EVT_VPLUS, 
NETCONN_EVT_RCVMINUS, 
NETCONN_EVT_SENDPLUS, 
NETCONN_EVT_SENDMINUS 
};


#ifdef MYOLD
#define SOCK_STREAM (1)
#define SOCK_DGRAM (2)
#define SOCK_RAW (3)
#define AF_INET (2)
#define PF_INET (2)
#endif

#define samemac(dest, host) (dest[0] == host[0] && dest[1] == host[1] && dest[2] == host[2] && dest[3] == host[3] && dest[4] == host[4] && dest[5] == host[5])
#define WORDSIZE (4)
#define SKB_DATA_ALIGN(X) (((X) + (WORDSIZE - 1)) &~ (WORDSIZE - 1))

extern struct skbuff *skb_alloc (unsigned int size);

extern void skb_free (struct skbuff *skb);

extern __u8 *skb_put (struct skbuff *skb, unsigned int len);

extern struct arpt_arp *arp_find_entry (__u32 ip);

extern int arp_input (struct skbuff **pskb, struct net_device *dev);

extern struct arpt_arp *update_arp_entry (__u32 ip, __u8 * ethaddr);

extern struct net_device *sip_init (void);

__u16 SIP_Chksum (void *dataptr, __u16 len);

#endif	/*__SIP_H__*/
