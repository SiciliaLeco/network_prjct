#include "network.h"
#include "mac.h"
#include "transport.h"
#include <stdio.h>
#include <string.h>

int checksum(uchar *buffer, uint size)
{
	uint sum = 0;
	uint cksum;
	uchar lower, higher;
	uint temp;
	while (size > 1)
	{
		lower = buffer[size - 1];
		higher = buffer[size - 2];
		temp = higher;
		temp = (temp << 8) + lower;
		sum += temp;
		size -= 2;
	}
	if (size == 1)
	{
		lower = buffer[size - 1];
		higher = 0x00;
		temp = higher;
		temp = (temp << 8) + lower;
		sum += temp;
	}
	cksum = (sum >> 16) + (sum & 0xffff);
	cksum = ~cksum;
	cksum = (cksum >> 8) + (cksum << 8);
	return cksum;
}

int verify_ip_checksum(int cksm_in_data, int cksm_cal)
{
	/*
		比较从mac提取到的checksum和计算的结果作对比
	*/
	if (cksm_in_data == cksm_cal)
		return 1;
	else
		return 0;
}

int verify_ip_addr(ipaddr srcip)
{
	/*
		check发来的数据是否目标是本机（192.168.0.2)
	*/
	if (srcip.seg1 != 192 || srcip.seg2 != 168 || srcip.seg3 != 0 || srcip.seg4 != 2)
		return 0;
	else
		return 1;
}
void send_ip(uchar *buf, ipaddr srcip, ipaddr dstip, uchar prtcl, uchar *payload, int len)
{
	/* 返回两个数值：
    	buf 将发送给mac层，作为其payload
    	return int 是payload的长度
    	这两个数值将在mac.c中的assemble()函数中被使用
       参数：
       	payload是来自传输层的内容
    */
	printf("-----ip layer receiving udp packet-----\n");
	iphdr header;
	uchar hdr[18];
	hdr[0] = header.version = 4;
	printf("ip version:%d\n", hdr[0]);
	hdr[1] = header.ihl = 5;
	printf("ip header length:%d\n", hdr[1]);
	hdr[2] = header.tos = 0;
	printf("ip type of service:%d\n", hdr[2]);
	hdr[3] = header.tot_len = 32 + len;
	printf("ip total length:%d\n", hdr[3]);
	hdr[4] = header.id = 0; // 可以发65535个不同id的datagram
	hdr[5] = header.frag_offset = 0;
	hdr[6] = header.ttl = 255;
	hdr[7] = header.protocol = prtcl;
	hdr[8] = header.checksum = 0;
	header.saddr = srcip; // my ip;
	header.daddr = dstip; // destination
	hdr[9] = header.saddr.seg1;
	hdr[10] = header.saddr.seg2;
	hdr[11] = header.saddr.seg3;
	hdr[12] = header.saddr.seg4;
	printf("src ip addr:%d.%d.%d.%d\n", hdr[9], hdr[10], hdr[11], hdr[12]);
	hdr[13] = header.daddr.seg1;
	hdr[14] = header.daddr.seg2;
	hdr[15] = header.daddr.seg3;
	hdr[16] = header.daddr.seg4;
	printf("dst ip addr:%d.%d.%d.%d\n", hdr[13], hdr[14], hdr[15], hdr[16]);
	hdr[17] = header.more_frag = 0;

	if (32 + len > mtu)
	{ //32= 2 ** 5
		printf("-------segmentation start-------\n");
		// exceeds, fragmentation
		int cfrag = len / mtu + 1; //如果要分片，片个数
		int i;
		for (i = 0; i < cfrag - 1; i++)
		{
			printf("-------segmentation generation: %d-------\n", i);
			hdr[3] = header.tot_len = 32 + mtu; //
			hdr[5] = header.frag_offset = i;	// MF
			hdr[17] = header.more_frag = 1;
			printf("ip frag offset:%d\n", hdr[5]);
			printf("ip more_frag: %d\n", hdr[17]);
			hdr[8] = header.checksum = checksum(hdr, 18);
			memcpy(&buf[0], hdr, 18);
			memcpy(&buf[18], payload + mtu * i, mtu);
			// for(int j =0; j< 18 + mtu; j++)
			// 	printf("%c", buf[j]);
			send_mac(18 + mtu, buf);
		}

		//最后一个分片，大小小于等于mtu
		printf("\n-------segmentation %d-------\n", i);
		hdr[3] = header.tot_len = 32 + len - mtu * (cfrag - 1);
		hdr[5] = header.frag_offset = i;
		hdr[17] = header.more_frag = 0;
		hdr[8] = header.checksum = checksum(hdr, 18);
		printf("ip frag offset:%d\n", hdr[5]);
		printf("ip more_frag: %d\n", hdr[17]);
		// printf("cksm for ip header:%d\n", checksum(hdr, 18));
		memcpy(&buf[0], hdr, 18);
		memcpy(&buf[18], payload + mtu * (cfrag - 1), len - mtu * (cfrag - 1));
		for (int j = 18; j < 18 + len - mtu * (cfrag - 1); j++)
			printf("%c", buf[j]);
		send_mac(18 + 2585 - mtu, buf);
	}

	else
	{
		// ok to send
		hdr[8] = header.checksum = checksum(hdr, 18);
		printf("ip checksum: %d\n", hdr[8]);
		memcpy(&buf[0], hdr, 18);
		memcpy(&buf[18], payload, len); // payload 装入
		printf("-----send ip info to mac layer-----\n");
		// for(int i =0; i < 20; i ++)
		// 	printf("%d\n", buf[i]); // 检查是不是都装入了
		send_mac(18 + len, buf);
	}
}

void send_mac(int len, char *buf)
{
	/*
		len是网络层数据包的长度(<mtu)
		buf是网络层的数据报内容
		本函数的功能是把网络层中的信息传递给mac层，进行mac和网络层的连接
	*/
	process_and_send_frame(len, buf);
}

void receive_ip(uchar *packet, uint len)
{
	/*
		packet：从mac层收到包，解析后把内容给到transport layer
		去掉ip层的头部信息，再给transport layer
	*/
	iphdr header;
	ipaddr srcip, dstip;
	uchar hdr[18];
	for (int i = 0; i < 18; i++)
		hdr[i] = packet[i];

	int cksm_data;
	printf("\n-----receiving data frame-----\n");
	header.version = hdr[0];
	header.ihl = hdr[1];
	printf("ip version:%d\n", hdr[0]);
	header.tos = hdr[2];
	header.tot_len = hdr[3];
	printf("ip header length:%d\n", hdr[1]);
	printf("ip type of service:%d\n", hdr[2]);
	header.id = hdr[4];
	header.frag_offset = hdr[5];
	printf("ip total length:%d\n", hdr[3]);
	printf("ip frag offset:%d\n", hdr[5]);
	header.ttl = hdr[6];
	header.protocol = hdr[7];
	cksm_data = header.checksum = hdr[8];
	header.saddr.seg1 = hdr[9];
	header.saddr.seg2 = hdr[10];
	header.saddr.seg3 = hdr[11];
	header.saddr.seg4 = hdr[12];
	header.daddr.seg1 = hdr[13];
	header.daddr.seg2 = hdr[14];
	header.daddr.seg3 = hdr[15];
	header.daddr.seg4 = hdr[16];
	header.more_frag = hdr[17];
	uchar *payload = packet + 18; // transport layer要收到的内容
	printf("src ip addr:%d.%d.%d.%d\n", hdr[9], hdr[10], hdr[11], hdr[12]);
	printf("dst ip addr:%d.%d.%d.%d\n", hdr[13], hdr[14], hdr[15], hdr[16]);
	printf("ip more_frag: %d\n", hdr[17]);
	hdr[8] = 0;
	// printf("receive_ip\n");
	// for(int i=0;i<18;i++){
	// 	printf("%d\n", hdr[i]);
	// }
	int cksm_cal = checksum(hdr, 18);

	if (!verify_ip_addr(header.daddr))
	{
		printf("not the dest addr!\n");
		printf("%d.%d.%d.%d\n", header.daddr.seg1, header.daddr.seg2, header.daddr.seg3, header.daddr.seg4);
		return;
	}

	if (verify_ip_checksum(cksm_cal, cksm_data))
	{
		printf("ip layer check error\n");
		return;
	}
	printf("declare: ip checksum ok\n");

	int more_frag = header.more_frag;
	int frag_offset = header.frag_offset;
	/*
		往传输层传信息，目前考虑：
			1. 因为有分片，所以考虑用文件整合了所有碎片，再一起发
			2. 如果没有分片，直接写入到文件
			3. udp处的接口应当为一个uchar，所以中间存在从文件到uchar的函数
			4. 当路由器把所有的分片拼接起来了，再调用send_transport_udp()函数将数据传给下一层
	*/
	int payload_len = hdr[3] - 32;
	int frag_len = 0;
	if (more_frag || frag_offset)
	{
		// 处理分片
		printf("fragment %d\n", frag_offset);
		FILE *ipfile = fopen(netfile, "a");
		fwrite(payload + 4, sizeof(char), more_frag == 0 ? 2585 - mtu : mtu, ipfile);
		fclose(ipfile);
		frag_len += payload_len;
		if (more_frag == 0)
		{
			// 没有更多分片，这个数据维护完毕，可以开始到udp进行处理了
			send_transport_udp(payload, 2585);
		}
	}
	else
	{
		printf("---------send datagram to transport------\n");
		FILE *ipfile = fopen(netfile, "w");
		fwrite(payload + 4, sizeof(char), payload_len - 4, ipfile); ////////////////////
		printf("end!\n");
		fclose(ipfile);
		send_transport_udp(payload, payload_len);
	}
}

void send_transport_udp(uchar *payload, int len)
{
	printf("-----send ip to transport layer-----\n");
	receive_udp(payload, len);
}
