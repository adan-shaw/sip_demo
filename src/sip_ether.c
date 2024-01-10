#include "sip.h"

static struct net_device ifdevice;

//静态函数前置声明(对外不暴露)
static int sip_init_ethnet (struct net_device *dev, struct net_device_info *info);
static __u8 input (struct skbuff *pskb, struct net_device *dev);
static __u8 output (struct skbuff *skb, struct net_device *dev);
static __u8 lowoutput (struct skbuff *skb, struct net_device *dev);

//公开函数(可通过extern 索取)
struct net_device *get_netif ()
{
	return &ifdevice;
}

struct net_device *sip_init (void)
{
	struct net_device_info info;
	strncpy(info.eth_name, "eno1", IFNAMSIZ);					//设置本机网卡设备别名
	strncpy(info.ip_host, "192.168.56.101", 32);			//设置本机IP地址
	strncpy(info.ip_gw, "192.168.56.1", 32);					//设置本机网关IP地址
	strncpy(info.ip_netmask, "255.255.255.0", 32);		//设置本机子网掩码地址
	strncpy(info.ip_multicast, "224.128.64.32", 32);	//设置目标多播IP地址
	if(sip_init_ethnet (&ifdevice,&info) == -1){			//初始化网络设备ifdevice
		printf("sip_init_ethnet() failed!!");
		return NULL;
	}
	else
		return &ifdevice;
}

//init 初始化 && 填充struct net_device {} 结构体, 成功返回0, 失败返回-1
static int sip_init_ethnet (struct net_device *dev, struct net_device_info *info)
{
	dev->s = socket (AF_INET, SOCK_PACKET, htons (ETH_P_ALL));									//建立一个SOCK_PACKET套接字
	if (dev->s > 0)
	{
		perror ("create(SOCK_PACKET)\n");
		return -1;
	}
	strncpy(&dev->info, info, sizeof(struct net_device_info));									//拷贝网卡设备string 描述信息

	dev->ip_host.s_addr = inet_addr (dev->info.ip_host);												//设置本机IP地址
	dev->ip_gw.s_addr = inet_addr (dev->info.ip_gw);														//设置本机网关IP地址
	dev->ip_netmask.s_addr = inet_addr (dev->info.ip_netmask);									//设置本机子网掩码地址
	dev->ip_multicast.s_addr = inet_addr (dev->info.ip_multicast);							//设置目标多播IP地址

	dev->to.sa_family = AF_INET;																								//设置'发送设备'的协议族
	strncpy(dev->to.sa_data, dev->ip_multicast.s_addr, sizeof(struct in_addr));	//设置addr地址结构sa_data = 目标多播IP地址
	bind (dev->s, &dev->to, sizeof (struct sockaddr));													//绑定套接字s到多播addr地址结构

	dev->hwaddr[0] = 0x00;																		//设置MAC地址
	dev->hwaddr[1] = 0x0c;
	dev->hwaddr[2] = 0x29;
	dev->hwaddr[3] = 0x73;
	dev->hwaddr[4] = 0x9D;
	dev->hwaddr[5] = 0x1F;

	memset (dev->hwbroadcast, 0xFF, ETH_ALEN);								//设置以太网的广播地址
	dev->hwaddr_len = ETH_ALEN;																//设置硬件地址长度
	dev->type = ETH_P_802_3;																	//设备的类型
	dev->input = input;																				//挂机以太网输入函数
	dev->output = output;																			//挂接以太网输出函数
	dev->linkoutput = lowoutput;															//挂接底层输出函数
	return 0;
}

void DISPLAY_MAC (struct ethhdr *eth)
{
	printf ("From:%02x-%02x-%02x-%02x-%02x-%02x          to:%02x-%02x-%02x-%02x-%02x-%02x\n", \
		eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5],
		eth->h_dest[0], eth->h_dest[1], eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
}

static __u8 input (struct skbuff *pskb, struct net_device *dev)
{
	char ef[ETH_FRAME_LEN];				//以太帧缓冲区,1514字节
	int n, i;
	struct skbuff *skb;

	//读取以太网数据, n为返回的实际捕获的以太帧的帧长
	n = read (dev->s, ef, ETH_FRAME_LEN);
	if (n <= 0)										//没有读到数据
	{
		printf ("Not datum\n");
		return -1;
	}
	else													//读到数据
		printf ("%d bytes datum\n", n);
	skb = skb_alloc (n);					//申请存放刚才读取到数据的空间
	if (!skb)											//申请失败
	{
		return -1;
	}
	memcpy (skb->head, ef, n);		//将接收到的网络数据拷贝到skb结构
	skb->tot_len = skb->len = n;	//设置长度值
	skb->phy.ethh = (struct sip_ethhdr *) skb_put (skb, sizeof (struct sip_ethhdr));//获得以太网头部指针
	//数据发往本机? 广播数据?
	if (samemac (skb->phy.ethh->h_dest, dev->hwaddr) || samemac (skb->phy.ethh->h_dest, dev->hwbroadcast))
	{
		switch (htons (skb->phy.ethh->h_proto))//查看以太网协议类型
		{
		case ETH_P_IP:							//IP类型
			skb->nh.iph = (struct sip_iphdr *) skb_put (skb, sizeof (struct sip_iphdr));//获得IP头部指针
			//将刚才接收到的网络数据用来更新ARP表中的映射关系
			arp_add_entry (skb->nh.iph->saddr, skb->phy.ethh->h_source, ARP_ESTABLISHED);
			ip_input (dev, skb);			//交给IP层处理数据
			break;
		case ETH_P_ARP:							//ARP类型
			//丢失了判断条件?
			{
				skb->nh.arph = (struct sip_arphdr *) skb_put (skb, sizeof (struct sip_arphdr));//获得ARP头部指针
				if (*((__be32 *) skb->nh.arph->ar_tip) == dev->ip_host.s_addr)//目的IP地址为本机?
				{
					arp_input (&skb, dev);//ARP模块处理接收到的ARP数据
				}
				skb_free (skb);					//释放内存
			}
			break;
		default:										//默认操作
			printf ("ETHER:UNKNOWN\n");
			skb_free (skb);						//释放内存
			break;
		}
	}
	else
	{
		skb_free (skb);							//释放内存
	}

	return 1;
}

static __u8 output (struct skbuff *skb, struct net_device *dev)
{
	struct arpt_arp *arp = NULL;
	int times = 0, found = 0;
	struct sip_ethhdr *eh;
	//发送网络数据的目的IP地址为skb所指的目的地址
	__be32 destip = skb->nh.iph->daddr;
	//判断目的主机和本机是否在同一个子网上
	if ((skb->nh.iph->daddr & dev->ip_netmask.s_addr) != (dev->ip_host.s_addr & dev->ip_netmask.s_addr))
	{
		destip = dev->ip_gw.s_addr;	//不在同一个子网上,将数据发送给网关
	}
	//分5次查找目的主机的MAC地址
	while ((arp = arp_find_entry (destip)) == NULL && times < 5)//查找MAC地址
	{
		arp_request (dev, destip);	//没有找到,发送ARP请求
		sleep (1);									//等一会
		times++;										//计数增加
	}
	if (!arp)											//没有找到对应的MAC地址
	{
		return -1;
	}
	else													//找到一个对应项
	{
		eh = skb->phy.ethh;
		memcpy (eh->h_dest, arp->ethaddr, ETH_ALEN);	//设置目的MAC地址为项中值
		memcpy (eh->h_source, dev->hwaddr, ETH_ALEN);	//设置源MAC地址为本机MAC值
		eh->h_proto = htons (ETH_P_IP);								//以太网的协议类型设置为IP
		dev->linkoutput (skb, dev);	//发送数据
	}
	return 1;
}

static __u8 lowoutput (struct skbuff *skb, struct net_device *dev)
{
	int n = 0;
	int len = sizeof (struct sockaddr);
	struct skbuff *p = NULL;
	//将skbuff链结构中的网络数据发送出去: 从skbuff的第一个结构开始, 到末尾一个结束, 发送完一个数据报文后移动指针并释放结构内存
	for (p = skb; p != NULL; skb = p, p = p->next, skb_free (skb), skb = NULL){
		n = sendto (dev->s, skb->head, skb->len, 0, &dev->to, len);//发送网络数据
		if(n == -1){
			perror("sendto()");
			return -1;
		}
		else
			printf ("sendto() bytes, n=%d\n", n);
	}
	return 0;
}



