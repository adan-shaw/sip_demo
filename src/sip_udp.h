#ifndef __SIP_UDP_H__
#define __SIP_UDP_H__

#define UDP_HTABLE_SIZE (128)		//UDP控制单元的大小
#define UDP_PORT_RANGE_START (4096)
#define UDP_PORT_RANGE_END (0x7fff)
#define UDP_FLAGS_NOCHKSUM (0x01U)
#define UDP_FLAGS_UDPLITE (0x02U)
#define UDP_FLAGS_CONNECTED (0x04U)

struct sip_udphdr
{
	__be16 source;								//源端口
	__be16 dest;									//目的端口
	__u16 len;										//数据长度
	__be16 check;									//UDP校验和
};

//Common members of all PCB types
struct ip_pcb
{
	struct in_addr ip_local;			//ip addresses in network byte order
	struct in_addr ip_remote;
	__u16 so_options;							//Socket options
	__u8 tos;											//Type Of Service
	__u8 ttl;											//Time To Live
	__u8 addr_hint;								//link layer address resolution hint
};

struct udp_pcb
{
	struct in_addr ip_local;			//本地IP地址
	__u16 port_local;							//本地端口地址
	struct in_addr ip_remote;			//发送目的IP地址
	__u16 port_remote;						//发送目的端口地址
	__u8 tos;											//服务类型
	__u8 ttl;											//生存时间
	__u8 flags;										//标记
	struct sock *sock;						//网络无关结构
	struct udp_pcb *next;					//下一个UDP控制单元
	struct udp_pcb *prev;					//前一个UDP控制单元
};

#endif
