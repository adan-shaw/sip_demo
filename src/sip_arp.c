#include "sip.h"

static struct arpt_arp arp_table[ARP_TABLE_SIZE];

void init_arp_entry ()
{
	int i = 0;
	for (i = 0; i < ARP_TABLE_SIZE; i++)	//初始化整个ARP映射表
	{
		arp_table[i].ctime = 0;			//初始时间值为0
		memset (arp_table[i].ethaddr, 0, ETH_ALEN);//MAC地址为0
		arp_table[i].ipaddr = 0;		//IP地址为0
		arp_table[i].status = ARP_EMPTY;		//表项状态为空
	}
}

struct arpt_arp *arp_find_entry (__u32 ip)
{
	int i = -1;
	struct arpt_arp *found = NULL;
	for (i = 0; i < ARP_TABLE_SIZE; i++)	//在ARP表中查找IP匹配项
	{
		if (arp_table[i].ctime > time (NULL) + ARP_LIVE_TIME)//查看是否表项超时
			arp_table[i].status = ARP_EMPTY;	//超时,置空表项
		//没有超时,查看是否IP地址匹配, 并且状态为已经建立
		else if (arp_table[i].ipaddr == ip && arp_table[i].status == ARP_ESTABLISHED)
		{
			found = &arp_table[i];		//找到一个合适的表项
			break;										//退出查找过程
		}
	}
	return found;
}

struct arpt_arp *update_arp_entry (__u32 ip, __u8 * ethaddr)
{
	struct arpt_arp *found = NULL;
	found = arp_find_entry (ip);	//根据IP查找ARP表项
	if (found)
	{															//找到对应表项
		memcpy (found->ethaddr, ethaddr, ETH_ALEN);//将给出的硬件地址拷贝到表项中
		found->status = ARP_ESTABLISHED;		//更新ARP的表项状态
		found->ctime = time (NULL);	//更新表项的最后更新时间
	}
	return found;
}

void arp_add_entry (__u32 ip, __u8 * ethaddr, int status)
{
	int i = 0;
	struct arpt_arp *found = NULL;
	found = update_arp_entry (ip, ethaddr);//更新ARP表项
	if (!found)										//更新不成功
	{															//查找一个空白表项将映射对写入
		for (i = 0; i < ARP_TABLE_SIZE; i++)
		{
			if (arp_table[i].status == ARP_EMPTY)//映射项为空
			{
				found = &arp_table[i];	//重置found变量
				break;									//退出查找
			}
		}
	}
	if (found)
	{															//对此项进行更新
		found->ipaddr = ip;					//IP地址更新
		memcpy (found->ethaddr, ethaddr, ETH_ALEN);//MAC地址更新
		found->status = status;			//状态更新
		found->ctime = time (NULL);	//最后更新时间更新
	}
}

//设备, ARP协议的类型, 源主机IP, 目的主机IP, 源主机MAC, 目的主机MAC, 解析的主机MAC
struct skbuff *arp_create (struct net_device *dev, int type, __u32 src_ip, __u32 dest_ip, __u8 * src_hw, __u8 * dest_hw, __u8 * target_hw)
{
	struct skbuff *skb;
	struct sip_arphdr *arph;
	DBGPRINT (DBG_LEVEL_TRACE, "==>arp_create\n");
	//请求skbuff结构内存,大小为一个最小的以太网帧,60字节
	skb = skb_alloc (ETH_ZLEN);
	if (skb == NULL)							//请求失败
	{
		goto EXITarp_create;				//退出
	}
	skb->phy.raw = skb_put (skb, sizeof (struct sip_ethhdr));	//更新物理层头部指针位置
	skb->nh.raw = skb_put (skb, sizeof (struct sip_arphdr));	//更新网络层头部指针位置
	arph = skb->nh.arph;					//设置ARP头部指针,便于操作
	skb->dev = dev;								//设置网络设备指针
	if (src_hw == NULL)						//以太网源地址为空
		src_hw = dev->hwaddr;				//源地址设置为网络设备的硬件地址
	if (dest_hw == NULL)					//以太网目的地址为空
		dest_hw = dev->hwbroadcast;	//目的地址设置为以太网广播硬件地址
	skb->phy.ethh->h_proto = htons (ETH_P_ARP);					//物理层网络协议设置为ARP协议
	memcpy (skb->phy.ethh->h_dest, dest_hw, ETH_ALEN);	//设置报文的目的硬件地址
	memcpy (skb->phy.ethh->h_source, src_hw, ETH_ALEN);	//设置报文的源硬件地址
	arph->ar_op = htons (type);		//设置ARP操作类型
	arph->ar_hrd = htons (ETH_P_802_3);//设置ARP的硬件地址类型为802.3
	arph->ar_pro = htons (ETH_P_IP);//设置ARP的协议地址类型为IP
	arph->ar_hln = ETH_ALEN;			//设置ARP头部的硬件地址长度为6
	arph->ar_pln = 4;							//设置ARP头部的协议地址长度为4
	memcpy (arph->ar_sha, src_hw, ETH_ALEN);						//ARP报文的源硬件地址
	memcpy (arph->ar_sip, (__u8 *) & src_ip, 4);				//ARP报文的源IP地址
	memcpy (arph->ar_tip, (__u8 *) & dest_ip, 4);				//ARP报文的目的IP地址
	if (target_hw != NULL)				//如果目的硬件地址不为空
		memcpy (arph->ar_tha, target_hw, dev->hwaddr_len);//ARP报文的目的硬件地址
	else													//没有给出目的硬件地址
		memset (arph->ar_tha, 0, dev->hwaddr_len);				//目的硬件地址留白

EXITarp_create:
	DBGPRINT (DBG_LEVEL_TRACE, "<==arp_create\n");
	return skb;
}

//设备, ARP协议的类型, 源主机IP, 目的主机IP, 源主机MAC, 目的主机MAC, 解析的主机MAC
void arp_send (struct net_device *dev, int type, __u32 src_ip, __u32 dest_ip, __u8 * src_hw, __u8 * dest_hw, __u8 * target_hw)
{
	struct skbuff *skb;
	DBGPRINT (DBG_LEVEL_TRACE, "==>arp_send\n");
	skb = arp_create (dev, type, src_ip, dest_ip, src_hw, dest_hw, target_hw);//建立一个ARP网络报文
	if (skb)
	{
		dev->linkoutput (skb, dev);									//建立成功: 调用底层的网络发送函数
	}
	DBGPRINT (DBG_LEVEL_TRACE, "<==arp_send\n");
}

void arp_request (struct net_device *dev, __u32 ip)
{
	struct skbuff *skb;
	DBGPRINT (DBG_LEVEL_TRACE, "==>arp_request\n");
	__u32 tip = 0;
	//查看请求的IP地址和本机IP地址是否在同一个自网上: 请求的IP地址 == 本机的IP地址
	if ((ip & dev->ip_netmask.s_addr) == (dev->ip_host.s_addr & dev->ip_netmask.s_addr))
	{
		tip = ip;																		//同一子网: 此IP为目的IP
	}
	else
	{
		tip = dev->ip_gw.s_addr;										//不同子网: 目的IP为网关地址
	}
	skb = arp_create (dev, ARPOP_REQUEST, dev->ip_host.s_addr, tip, dev->hwaddr, NULL, NULL);//建立一个ARP请求报文,其中的目的IP为上述地址
	if (skb)
	{
		dev->linkoutput (skb, dev);									//建立skbuff成功: 通过底层网络函数发送
	}
	DBGPRINT (DBG_LEVEL_TRACE, "<==arp_request\n");
}

int arp_input (struct skbuff **pskb, struct net_device *dev)
{
	struct in_addr t_addr;
	struct skbuff *skb = *pskb;
	__be32 ip = 0;
	DBGPRINT (DBG_LEVEL_TRACE, "==>arp_input\n");
	if (skb->tot_len < sizeof (struct sip_arphdr))//接收到的网络数据总长度小于ARP头部长度
	{
		return 0;
	}
	ip = *(__be32 *) (skb->nh.arph->ar_tip);			//ARP请求的目的地址
	if (ip == dev->ip_host.s_addr)								//为本机IP?
	{
		update_arp_entry (ip, dev->hwaddr);					//更新ARP表
	}
	switch (ntohs (skb->nh.arph->ar_op))					//查看ARP头部协议类型
	{
	case ARPOP_REQUEST:														//ARP请求类型
			t_addr.s_addr = *(unsigned int *) skb->nh.arph->ar_sip;//填写ARP请求源IP地址
			DBGPRINT (DBG_LEVEL_ERROR, "ARPOP_REQUEST, FROM:%s\n", inet_ntoa (t_addr));
			arp_send (dev, ARPOP_REPLY, dev->ip_host.s_addr, *(__u32 *) skb->nh.arph->ar_sip, dev->hwaddr, skb->phy.ethh->h_source, skb->nh.arph->ar_sha);//向ARP请求的IP地址发送应答
			arp_add_entry (*(__u32 *) skb->nh.arph->ar_sip, skb->phy.ethh->h_source, ARP_ESTABLISHED);//将此项ARP映射内容加入映射表
		break;
	case ARPOP_REPLY:															//ARP应答类型
		arp_add_entry (*(__u32 *) skb->nh.arph->ar_sip, skb->phy.ethh->h_source, ARP_ESTABLISHED);//将此项ARP映射内容加入映射表
		break;
	}
	DBGPRINT (DBG_LEVEL_TRACE, "<==arp_input\n");

	return 1;
}
