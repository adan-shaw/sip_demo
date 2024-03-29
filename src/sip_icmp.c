#include "sip.h"

extern int IP_IS_BROADCAST (struct net_device *dev, __be32 ip);

static void icmp_echo (struct net_device *dev, struct skbuff *skb)
{
	struct sip_icmphdr *icmph = skb->th.icmph;
	struct sip_iphdr *iph = skb->nh.iph;
	printf ("tot_len:%d\n", skb->tot_len);

	//判断目的IP地址是否广播, 判断目的IP地址是否多播
	if (IP_IS_BROADCAST (dev, skb->nh.iph->daddr) || IP_IS_MULTICAST (skb->nh.iph->daddr))
	{
		return;
	}
	icmph->type = ICMP_ECHOREPLY;//设置类型为回显应答
	if (icmph->checksum >= htons (0xFFFF - (ICMP_ECHO << 8)))//如果因为修改协议类型造成进位
	{
		icmph->checksum += htons (ICMP_ECHO << 8) + 1;//修正校验和
	}
	else
	{
		icmph->checksum += htons (ICMP_ECHO << 8);//增加校验和
	}
	__be32 dest = skb->nh.iph->saddr;
	ip_output (dev, skb, &dev->ip_host.s_addr, &dest, 255, 0, IPPROTO_ICMP);//发送应答

	return;
}

static void icmp_discard (struct net_device *dev, struct skbuff *skb)
{
	;
}

static void icmp_unreach (struct net_device *dev, struct skbuff *skb)
{
	/*
	struct skbuff *q;
	struct ip_hdr *iphdr;
	struct icmp_dur_hdr *idur;
	//ICMP header + IP header + 8 bytes of data
	q = skbuff_alloc (PBUF_IP, sizeof (struct icmp_dur_hdr) + IP_HLEN + ICMP_DEST_UNREACH_DATASIZE, PBUF_RAM);
	if (q == NULL)
	{
		LWIP_DEBUGF (ICMP_DEBUG, ("icmp_dest_unreach: failed to allocate skbuff for ICMP packet.\n"));
		return;
	}
	LWIP_ASSERT ("check that first skbuff can hold icmp message", (q->len >= (sizeof (struct icmp_dur_hdr) + IP_HLEN + ICMP_DEST_UNREACH_DATASIZE)));
	iphdr = p->payload;
	idur = q->payload;
	ICMPH_TYPE_SET (idur, ICMP_DUR);
	ICMPH_CODE_SET (idur, t);
	SMEMCPY ((u8_t *) q->payload + sizeof (struct icmp_dur_hdr), p->payload, IP_HLEN + ICMP_DEST_UNREACH_DATASIZE);
	//calculate checksum
	idur->chksum = 0;
	idur->chksum = inet_chksum (idur, q->len);
	ICMP_STATS_INC (icmp.xmit);
	//increase number of messages attempted to send
	snmp_inc_icmpoutmsgs ();
	//increase number of destination unreachable messages attempted to send
	snmp_inc_icmpoutdestunreachs ();
	ip_output (q, NULL, &(iphdr->src), ICMP_TTL, 0, IP_PROTO_ICMP);
	skb_free (q);
	*/
}

static void icmp_redirect (struct net_device *dev, struct skbuff *skb)
{
	printf ("==>icmp_redirect\n");
	printf ("<==icmp_redirect\n");
}

static void icmp_timestamp (struct net_device *dev, struct skbuff *skb)
{
	printf ("==>icmp_timestamp\n");
	printf ("<==icmp_timestamp\n");
}

static void icmp_address (struct net_device *dev, struct skbuff *skb)
{
	printf ("==>icmp_address\n");
	printf ("<==icmp_address\n");
}

static void icmp_address_reply (struct net_device *dev, struct skbuff *skb)
{
	printf ("==>icmp_address_reply\n");
	printf ("<==icmp_address_reply\n");
}

//This table is the definition of how we handle ICMP.

static const struct icmp_control icmp_pointers[NR_ICMP_TYPES + 1] = {
	[ICMP_ECHOREPLY] = {					//回显应答
			.handler = icmp_discard,	//丢弃
		},
	[1] = {
			.handler = icmp_discard,
			.error = 1,
		},
	[2] = {
			.handler = icmp_discard,
			.error = 1,
		},
	[ICMP_DEST_UNREACH] = {				//主机不可达
			.handler = icmp_unreach,
			.error = 1,
		},
	[ICMP_SOURCE_QUENCH] = {			//源队列
			.handler = icmp_unreach,
			.error = 1,
		},
	[ICMP_REDIRECT] = {						//重定向
			.handler = icmp_redirect,
			.error = 1,
		},
	[6] = {
			.handler = icmp_discard,
			.error = 1,
		},
	[7] = {
			.handler = icmp_discard,
			.error = 1,
		},
	[ICMP_ECHO] = {								//回显应答
			.handler = icmp_echo,
		},
	[9] = {
			.handler = icmp_discard,
			.error = 1,
		},
	[10] = {
			.handler = icmp_discard,
			.error = 1,
		},
	[ICMP_TIME_EXCEEDED] = {			//时间超时
			.handler = icmp_unreach,
			.error = 1,
		},
	[ICMP_PARAMETERPROB] = {			//参数有误
			.handler = icmp_unreach,
			.error = 1,
		},
	[ICMP_TIMESTAMP] = {					//时间戳请求
			.handler = icmp_timestamp,
		},
	[ICMP_TIMESTAMPREPLY] = {			//时间戳应答
			.handler = icmp_discard,
		},
	[ICMP_INFO_REQUEST] = {				//信息请求
			.handler = icmp_discard,
		},
	[ICMP_INFO_REPLY] = {					//信息应答
			.handler = icmp_discard,
		},
	[ICMP_ADDRESS] = {						//IP地址掩码请求
			.handler = icmp_address,
		},
	[ICMP_ADDRESSREPLY] = {				//IP地址掩码应答
			.handler = icmp_address_reply,
		}
};

int icmp_reply (struct net_device *dev, struct skbuff *skb)
{

}

//Deal with incoming ICMP packets.

int icmp_input (struct net_device *dev, struct skbuff *skb)
{
	struct sip_icmphdr *icmph;
	switch (skb->ip_summed)				//查看是否已经进行了校验和计算
	{
	case CHECKSUM_NONE:						//没有计算校验和
		skb->csum = 0;
		if (SIP_Chksum (skb->phy.raw, 0))//计算IP层的校验和
		{
			printf ("icmp_checksum error\n");
			return -1;
		}
		break;
	default:
		break;
	}
	icmph = skb->th.icmph;				//ICMP头指针
	if (icmph->type > NR_ICMP_TYPES){//类型不对
		return -1;
	}
	icmp_pointers[icmph->type].handler (dev, skb);//查找icmp_pointers中合适类型的处理函数

	return 0;
}
