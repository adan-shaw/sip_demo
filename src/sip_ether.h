#ifndef __SIP_ETHER_H__
#define __SIP_ETHER_H__

//中文备注声明: linux 内核头文件中宏定义
/*
#define ETH_P_IP (0x0800)		//IP类型报文
#define ETH_P_ARP (0x0806)	//ARP报文
#define ETH_ALEN (6)				//以太网地址长度
#define ETH_HLEN (14)				//以太网头部长度
#define ETH_ZLEN (60)				//以太网最小长度
#define ETH_DATA_LEN (1500)	//以太网的最大负载长度
#define ETH_FRAME_LEN (1514)//以太网最大长度
#define ETH_P_ALL (0x0003)	//使用SOCK_PACKET获取每一个包
*/

//This is an Ethernet frame header.
struct sip_ethhdr
{
	__u8 h_dest[ETH_ALEN];				//目的以太网地址
	__u8 h_source[ETH_ALEN];			//源以太网地址
	__be16 h_proto;								//数据包的类型
};

struct skbuff;

struct net_device_info
{
	char eth_name[IFNAMSIZ];			//本机网卡设备别名
	char ip_host[32];							//本机IP地址
	char ip_gw[32];								//本机网关IP地址
	char ip_netmask[32];					//本机子网掩码地址
	char ip_multicast[32];				//目标多播IP地址
};
struct net_device
{
	struct net_device_info info;	//网卡设备string 描述信息
	struct in_addr ip_host;				//本机IP地址
	struct in_addr ip_gw;					//本机网关IP地址
	struct in_addr ip_netmask;		//本机子网掩码
	struct in_addr ip_multicast;	//目标多播IP地址
	struct sockaddr to;						//目标多播addr地址结构
	int s;												//建立的套接字描述符
	__u16 type;										//发送类型
	__u8 (*input) (struct skbuff * skb, struct net_device * dev);			//这个函数用于从网络设备中获取数据传入网络协议栈进行处理
	__u8 (*output) (struct skbuff * skb, struct net_device * dev);		//这个函数用于IP模块发送数据时候调用此函数会先调用ARP模块对IP地址进行查找, 然后发送数据
	__u8 (*linkoutput) (struct skbuff * skb, struct net_device * dev);//这个函数由ARP模块调用,直接发送网络数据
	__u8 hwaddr_len;							//硬件地址的长度
	__u8 hwaddr[ETH_ALEN];				//硬件地址的值,MAC
	__u8 hwbroadcast[ETH_ALEN];		//硬件的广播地址
	__u8 mtu;											//网卡的最大传输长度
};

#endif
