#ifndef MAC_H_
#define MAC_H_
#include <stdio.h>
#include <string.h>
#define uchar unsigned char
#define uint unsigned int 
#define ifile "test.txt" // 宏定义输出帧的文件夹

uint crc32_table[256];
void make_crc32_table();
uint crc32(uchar *string, uint size);
int assemble(uchar *frame, uchar *payload, int paylen);
void mac_sender(FILE *sendfile, uchar *frame, uint len);
int checkCRC(uchar *crcGet, uchar *crcCal);
int disassemble(FILE *sendfile, uchar *frame, uchar *paylaod);
void mac_receiver();
void process_and_send_frame(int len, uchar *buf);
#endif