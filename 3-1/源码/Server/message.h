#pragma once

#include <winsock2.h>
#include <stdint.h>
#include <iostream>
#include <sstream>

using namespace std;

#define ACK 4
#define SYN 2
#define SYN_ACK 6
#define FIN 1
#define FIN_ACK 5
#define HEAD 8
#define TAIL 16
#define MES_SIZE 1054

//设计报文头部格式
//以下内容按1byte对齐，如果没有这条指令会以4Byte对齐
class message {
public:
#pragma pack(1)
	uint32_t FLAGS;		//标志位
	uint32_t sendIP;	//发送端IP
	uint32_t recvIP;	//接收端IP
	uint16_t sendPort;	//发送端口
	uint16_t recvPort;	//接收端口
	uint32_t messeq;	//消息序号
	uint32_t ackseq;	//确认序号
	uint32_t filelen;	//文件长度
	//在第一个数据包时表示整个文件的长度，后面表示单个数据包中数据的长度
	uint16_t checksum;	//校验和
	char data[1024];	//文件数据
#pragma pack()
	message() :FLAGS(0), sendIP(0), recvIP(0), sendPort(0), recvPort(0), messeq(0), ackseq(0), filelen(0), checksum(0) {
		memset(data, 0, 1024);
	}
	message(uint32_t FLAGS, uint32_t sendIP, uint32_t recvIP, u_short sendPort, u_short recvPort, uint32_t messeq, uint32_t ackseq, uint32_t filelen, uint32_t checksum);
	void setHEAD(int messeq, int fileSize, char* fileName);
	void fillData(int messeq, int size, char* data);	//填充数据
	void setchecksum(uint16_t* mes);
	void clearMes();
};

message::message(uint32_t FLAGS, uint32_t sendIP, uint32_t recvIP, u_short sendPort, u_short recvPort, uint32_t messeq, uint32_t ackseq, uint32_t filelen, uint32_t checksum) {
	this->FLAGS = FLAGS;
	this->sendIP = sendIP;
	this->recvIP = recvIP;
	this->sendPort = sendPort;
	this->recvPort = recvPort;
	this->messeq = messeq;
	this->ackseq = ackseq;
	this->filelen = filelen;
	this->checksum = checksum;
	memset(this->data, 0, sizeof(this->data));
}

//void message::setACK() {
//	//设置ACK位为1
//	this->FLAGS = ACK;
//	//FLAGS -> 00000100
//}
//
//void message::setSYN() {
//	//设置SYN位为1
//	this->FLAGS = SYN;
//	//FLAGS -> 00000010
//}
//
//void message::setSYNACK() {
//	//设置ACK，SYN位为1
//	this->FLAGS = SYN_ACK;
//	//FLAGS -> 00000110
//}
//
//void message::setFIN() {
//	//设置FIN位为1
//	this->FLAGS = FIN;
//	//FLAGS -> 00000001
//}
//
//void message::setFINACK() {
//	//设置FIN,ACK位为1
//	this->FLAGS = FIN_ACK;
//	//FLAGS -> 00000101
//}

void message::setHEAD(int messeq, int fileSize, char* fileName) {
	//设置HEAD位为1
	this->FLAGS = HEAD; //FLAGS->00001000
	this->filelen = fileSize;
	this->messeq = messeq;
	memcpy(this->data, fileName, strlen(fileName) + 1);
}

//void message::setTAIL() {
//	//设置TAIL位为1
//	this->FLAGS = TAIL;
//	//FLAGS -> 00010000
//}

void message::fillData(int messeq, int size, char* data) {
	//将文件数据填入数据包data变量
	this->messeq = messeq;
	this->filelen = size;
	memcpy(this->data, data, size);
}

//计算校验和
uint16_t calChecksum(uint16_t* mes) {
	int size = MES_SIZE;	//得到数据的大小(单位为字节)
	int count = size / 2 - 1;	//每次循环计算2字节
	uint16_t* buf = (uint16_t*)malloc(size);	//用于遍历数据的缓冲区
	memset(buf, 0, size);
	memcpy(buf, mes, size); //复制
	uint32_t sum = 0;
	while (count--) {
		sum += *buf; //累加
		buf++;
		//如果sum的高16位不为0，则
		if (sum & 0xFFFF0000) {
			//存储高16位；
			sum &= 0xFFFF;
			sum++;
		}
	}
	return ~(sum & 0xFFFF);
}

//设置校验和
void message::setchecksum(uint16_t* mes) {
	this->checksum = calChecksum(mes); //按位取反，方便计算
}

//判断文件是否损坏
bool isCorrupt(message* mes) {
	if (calChecksum((uint16_t*)mes) == 0) {
		return false;
	}
	return true;
}

//打印数据包消息
void printMessage(message* Mes) {
	cout << "数据包大小=" << Mes->filelen << "比特，标志位=" << Mes->FLAGS << endl;
	cout << "发送序号=" << Mes->messeq << "确认序号=" << Mes->ackseq << endl;
	cout << "校验和=" << Mes->checksum << endl;
}

//打印发送数据包消息
void printSendMessage(message* Mes) {
	cout << "【发送包信息】" << endl;
	printMessage(Mes);
}

//打印接收数据包消息
void printRecvMessage(message* Mes) {
	cout << "【接收包消息】" << endl;
	printMessage(Mes);
}

//判断发送序号
bool has_seq1(message* Mes) {
	if (Mes->messeq == 1) {
		return true;
	}
	return false;
}

//判断发送序号
bool has_seq0(message* Mes) {
	if (Mes->messeq == 0) {
		return true;
	}
	return false;
}

//判断ACK序号
bool isACK(message* Mes, int ack) {
	if (Mes->ackseq == ack && Mes->FLAGS == ACK) {
		return true;
	}
	return false;
}



//打印系统时间
void printTime() {
	cout << endl;
	stringstream ss;	//声明一个stringstream对象ss，用于将数据转换为字符串
	SYSTEMTIME sysTime = { 0 };	//结构体SYSTEMTIME，用于保存系统时间的各个组成部分
	ss.clear();
	ss.str("");	//清空stringstream
	//获取当前系统时间，并存储在sysTime结构体中
	GetSystemTime(&sysTime);
	//将时间写入stringstream对象ss
	ss << "[" << sysTime.wYear << "/" << sysTime.wMonth << "/" << sysTime.wDay
		<< " " << sysTime.wHour + 8 << ":" << sysTime.wMinute << ":"
		<< sysTime.wSecond << ":" << sysTime.wMilliseconds << "]";
	cout << "时间：" << ss.str() << endl;
}

//清空消息结构
void message::clearMes() {
	this->FLAGS = this->sendIP = this->recvIP = this->sendPort = this->recvPort = 0;
	this->ackseq = this->messeq = this->filelen = this->checksum = 0;
	memset(data, 0, sizeof(data));
}