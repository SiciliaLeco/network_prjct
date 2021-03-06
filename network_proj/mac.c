#include <stdio.h>
#include <string.h>
#include "mac.h"
#include "network.h"

void make_crc32_table() // crc检验表生成
{
	uint c;
	int i = 0;
	int bit = 0;

	for (i = 0; i < 256; i++)
	{
		c = (uint)i;

		for (bit = 0; bit < 8; bit++)
		{
			if (c & 1)
			{
				c = (c >> 1) ^ (0xEDB88320);
			}
			else
			{
				c = c >> 1;
			}
		}
		crc32_table[i] = c;
	}
}

uint crc32(uchar *string, uint size) // crc32检测，表驱动
{
	make_crc32_table();
	uint crc = 0xffffffff;
	while (size--)
		crc = (crc >> 8) ^ (crc32_table[(crc ^ *string++) & 0xff]);
	return crc;
}

int assemble(uchar *frame, uchar *payload, int paylen)
{
	/*生成帧，payload为数据，
	paylen是数据长度，函数返回frame是生成的帧
	*/
	if (paylen < 64 || paylen > 1518) // 整个帧的范围控制
		return -1;
	printf("----assemble start!----\n");
	uchar dst[6] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46}; // 目标地址
	printf("mac dst addr: %x %x %x %x %x %x\n", dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]);
	uchar src[6] = {0x47, 0x42, 0x43, 0x44, 0x45, 0x46}; // 出发地址
	printf("mac src addr: %x %x %x %x %x %x\n", src[0], src[1], src[2], src[3], src[4], src[5]);
	uchar ptype[2] = {0x47, 0x42}; // protocol type
	memcpy(&frame[0], dst, 6);
	memcpy(&frame[6], src, 6);
	memcpy(&frame[12], &ptype, 2);
	memcpy(&frame[14], payload, paylen);
	uint crc = crc32(frame, paylen + 14); // 生成crc检验
	memcpy(&frame[14 + paylen], &crc, 4);
	printf("----frame packed----\n");
	return 14 + paylen + 4; // 返回整个帧的长度
}

void mac_sender(FILE *sendfile, uchar *frame, uint len)
{
	// send frame as a file
	fwrite(&len, sizeof(len), 1, sendfile); // length
	fwrite(frame, sizeof(char), len, sendfile);
	fclose(sendfile); //用文件的形式传输
	printf("-----finish sending frame!-----\n");
	printf("\n\n\n\n");

	mac_receiver();
}

int checkCRC(uchar *crcGet, uchar *crcCal)
{
	/* Get是帧送来的，Cal是帧算来的
		检查CRC是否出错，如果没有错误就可以继续
	*/
	for (int i = 0; i < 4; i++)
	{
		if (crcGet[i] != crcCal[i]) // 验证出来出错了
			return -1;
	}
	return 1;
}

int disassemble(FILE *sendfile, uchar *frame, uchar *payload)
{
	/* 拆帧，把文件中的帧解码 */
	printf("----disassemble start!----\n");
	uint len;
	int plen = 0;
	while (fread(&len, sizeof(len), 1, sendfile))
	{
		// printf("Frame_length:%d\n", len);
		fread(frame, sizeof(char), len, sendfile);
		//按长度读取
		uchar dst[6];
		uchar src[6];
		uchar type[2];
		uchar crcBuff[4], crcSent[4];
		uint crc = crc32(frame, len - 4);
		// printf("crc_receive(in dec format): %d\n", crc);
		// printf("crc_receive(in hex format): %x\n", crc);
		crcSent[3] = frame[len - 1];
		crcSent[2] = frame[len - 2];
		crcSent[1] = frame[len - 3];
		crcSent[0] = frame[len - 4];
		memcpy(crcBuff, &crc, sizeof(crc));
		if (checkCRC(crcBuff, crcSent) < 0)
		{
			printf("Bitstream broken! receive failed.\n");
			return -1;
		}
		else
		{
			printf("receive message succeed\n");
			// 打印帧信息
			for (int i = 0; i < len - 4; i++)
			{
				if (i >= 0 && i < 6)
				{
					if (i == 0)
						printf("dest Addr: ");
					printf("%x ", frame[i]);
				}
				else if (i < 12)
				{
					if (i == 6)
						printf("\nsrc Addr: ");
					printf("%x ", frame[i]);
				}
				else if (i < 14)
				{
					if (i == 12)
						printf("\nProtocol Type: ");
					printf("%d ", frame[i]);
				}
				else
				{
					// if(i == 14)
					// printf("\nPayload: ");
					// printf("%d", frame[i]);
					payload[plen++] = frame[i];
				}
			}
		}
	}
	return plen;
}

void sent_to_ip()
{
	FILE *receivefile = fopen(ifile, "r");
	uchar frame[1600];
	uchar payload[1600];
	int plen = disassemble(receivefile, frame, &payload[0]);
	//要用到这里的frame取出来的payload，给ip层去
	// for(int i = 0; i < plen; i++)
	// 	printf("%d\n", payload[i]);
	receive_ip(payload, plen);
	fclose(receivefile);
}

void mac_receiver()
{
	/* 对写的传输帧的功能进行检测， receivefile是文件形式的帧 */

	printf("-------rececving data from net...--------\n");
	FILE *receivefile = fopen(ifile, "r");
	uchar frame[1600];
	uchar payload[1600];
	int len = disassemble(receivefile, frame, &payload[0]);
	fclose(receivefile);
	receive_ip(payload, len);
}

void process_and_send_frame(int len, uchar *buf)
{
	uchar frame[1600];
	int a = assemble(&frame[0], buf, len);
	if (a < 0)
	{
		printf("assemble denied: length wrong!\n");
		return;
	}
	FILE *writefile = fopen(ifile, "w");
	mac_sender(writefile, frame, a);
}
