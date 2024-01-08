#ifndef __SIP_SOCKET_H__

#define __SIP_SOCKET_H__

//Contains all internal pointers and states used for a socket
struct sip_socket
{
	struct sock *sock;			//协议无关层的结构指针,一个socket对应一个sock
	struct skbuff *lastdata;//最后接收的网络数据
	__u16 lastoffset;				//接收的网络数据偏移量,这是由于不能一次将网络数据拷贝给用户造成的
	int err;								//错误值
};

#endif	/*__SIP_SOCKET_H__*/
