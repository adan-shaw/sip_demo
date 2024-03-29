#include "sip.h"
#include "sip_icmp.h"

#define NUM_SOCKETS (4)

static struct sip_socket sockets[NUM_SOCKETS];

/*
 * Map a externally used socket index to the internal socket representation.
 *
 * @param s externally used socket index
 * @return struct sip_socket for the socket or NULL if not found
*/
static struct sip_socket *get_socket (int s)
{
	struct sip_socket *socket = &sockets[s % NUM_SOCKETS];
	if ((s < 0) || (s >= NUM_SOCKETS))
		return NULL;
	if (!socket->sock)
		return NULL;
	else if (socket->sock->socket != s)
		return NULL;
	else
		return socket;
}

/*
 * Allocate a new socket for a given sock.
 *
 * @param newsock the sock for which to allocate a socket
 * @return the index of the new socket; -1 on error
*/
static int alloc_socket (struct sock *newsock)
{
	int i;
	for (i = 0; i < NUM_SOCKETS; ++i)//allocate a new socket identifier
	{
		if (!sockets[i].sock)
		{
			sockets[i].sock = newsock;
			sockets[i].lastdata = NULL;
			sockets[i].lastoffset = 0;
			sockets[i].err = 0;
			return i;
		}
	}
	return -1;
}

int sip_socket (int domain, int type, int protocol)
{
	struct sock *p;
	int pos;
	if (domain != AF_INET || protocol != 0)	//协议类型不对
		return -1;
	switch (type)									//按照类型建立不同的套接字
	{
	case SOCK_DGRAM:
		p = (struct sock *) SIP_SockNew (SOCK_DGRAM);//建立udp套接字
		break;
	case SOCK_STREAM:
		p = (struct sock *) SIP_SockNew (SOCK_STREAM);//建立tcp套接字
		break;
	default:
		return -1;
	}
	if (!p)
		return -1;									//建立套接字失败

	pos = alloc_socket (p);				//初始化socket变量,并分配文件描述符
	if (pos == -1)
	{															//上述操作失败
		SIP_SockDelete (p);					//释放sock类型变量
		return -1;
	}
	p->socket = pos;							//设置sock结构中的socket值
	return p;
}

int sip_close (int s)
{
	struct sip_socket *socket = get_socket (s);//获得socket类型映射
	if (!socket)									//失败
		return -1;
	SIP_SockDelete (socket->sock);//释放sock结构
	if (socket->lastdata)
		skb_free (socket->lastdata);//释放socket上挂接的网络数据
	socket->lastdata = NULL;			//清空socket结构的网络数据
	socket->sock = NULL;					//清空sock指针
	return 0;
}

int sip_bind (int sockfd, const struct sockaddr *my_addr, socklen_t addrlen)
{
	struct in_addr local_addr;
	__u16 port_local;
	struct sip_socket *socket = get_socket (sockfd);	//获得socket类型映射
	if (!socket)
		return -1;
	local_addr.s_addr = ((struct sockaddr_in *) my_addr)->sin_addr.s_addr;
	port_local = ((struct sockaddr_in *) my_addr)->sin_port;
	if ((SIP_SockBind (socket->sock, &local_addr, ntohs (port_local))) != 0)//协议无关层的绑定函数
		return -1;
	return 0;
}

int sip_connect (int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
	struct sip_socket *socket = get_socket (sockfd);	//获得socket类型映射
	if (!socket)
		return -1;
	struct in_addr remote_addr;
	__u16 remote_port;
	remote_addr.s_addr = ((struct sockaddr_in *) serv_addr)->sin_addr.s_addr;
	remote_port = ((struct sockaddr_in *) serv_addr)->sin_port;
	if (SIP_SockConnect (socket->sock, &remote_addr, ntohs (remote_port)))
		return -1;
	return 0;
}

ssize_t sip_recvfrom (int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t * fromlen)
{
	struct skbuff *skb;
	struct sockaddr_in *f = (struct sockaddr_in *) from;
	int len_copy = 0;
	struct sip_socket *socket = get_socket (s);//获得socket类型映射
	if (!socket)
		return -1;
	if (!socket->lastdata)
	{															//lastdata中没有有剩余数据
		socket->lastdata = (struct skbuff *) SIP_SockRecv (socket->sock);	//接收数据
		socket->lastoffset = 0;			//偏离量为0
	}
	skb = socket->lastdata;				//skbuff指针
	//填充用户出入参数
	*fromlen = sizeof (struct sockaddr_in);	//地址结构长度
	f->sin_family = AF_INET;			//地址类型
	f->sin_addr.s_addr = skb->nh.iph->saddr;	//来源IP地址
	f->sin_port = skb->th.udph->source;	//来源端口
	len_copy = skb->len - socket->lastoffset;	//计算lastdata中剩余的数据
	if (len > len_copy)
	{															//用户缓冲区可以放下所有数据
		memcpy (buf, skb->data + socket->lastoffset, len_copy);//全部拷贝到用户缓冲区
		skb_free (skb);							//释放此结构
		socket->lastdata = NULL;		//清空网络数据结构指针
		socket->lastoffset = 0;			//偏移量重新设置为0
	}
	else
	{															//用户缓冲区放不下整个数据
		len_copy = len;							//仅拷贝缓冲区大小的数据
		memcpy (buf, skb + socket->lastoffset, len_copy);	//拷贝
		socket->lastoffset += len_copy;	//偏移量增加
	}
	return len_copy;							//返回拷贝的值
}

ssize_t sip_recv (int s, void *buf, size_t len, int flags)
{
	return sip_recvfrom (s, buf, len, flags, NULL, NULL);
}

ssize_t sip_sendto (int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
	struct sip_socket *socket;
	struct in_addr remote_addr;
	struct sockaddr_in *to_in = (struct sockaddr_in *) to;
	//网络数据头部的长度
	int l_head = sizeof (struct sip_ethhdr) + sizeof (struct sip_iphdr) + sizeof (struct sip_udphdr);
	int size = l_head + len;			//数据总长度
	struct skbuff *skb = skb_alloc (size);	//申请空间
	char *data = skb_put (skb, l_head);	//设置data指针
	memcpy (data, buf, len);			//将用户数据拷贝到缓冲区
	remote_addr = to_in->sin_addr;	//设置目的IP地址
	socket = get_socket (s);
	if (!socket)
		return -1;
	SIP_SockSendTo (socket->sock, skb, &remote_addr, to_in->sin_port);	//发送数据
	return len;
}

ssize_t sip_send (int s, const void *buf, size_t len, int flags)
{
	struct sip_socket *socket = get_socket (s);
	if (!socket)
		return -1;
	return sip_sendto (s, buf, len, flags, NULL, 0);
}
