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

//��Ʊ���ͷ����ʽ
//�������ݰ�1byte���룬���û������ָ�����4Byte����
class message {
public:
#pragma pack(1)
	uint32_t FLAGS;		//��־λ
	uint32_t sendIP;	//���Ͷ�IP
	uint32_t recvIP;	//���ն�IP
	uint16_t sendPort;	//���Ͷ˿�
	uint16_t recvPort;	//���ն˿�
	uint32_t messeq;	//��Ϣ���
	uint32_t ackseq;	//ȷ�����
	uint32_t filelen;	//�ļ�����
	//�ڵ�һ�����ݰ�ʱ��ʾ�����ļ��ĳ��ȣ������ʾ�������ݰ������ݵĳ���
	uint16_t checksum;	//У���
	char data[1024];	//�ļ�����
#pragma pack()
	message() :FLAGS(0), sendIP(0), recvIP(0), sendPort(0), recvPort(0), messeq(0), ackseq(0), filelen(0), checksum(0) {
		memset(data, 0, 1024);
	}
	message(uint32_t FLAGS, uint32_t sendIP, uint32_t recvIP, u_short sendPort, u_short recvPort, uint32_t messeq, uint32_t ackseq, uint32_t filelen, uint32_t checksum);
	void setHEAD(int messeq, int fileSize, char* fileName);
	void fillData(int messeq, int size, char* data);	//�������
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
//	//����ACKλΪ1
//	this->FLAGS = ACK;
//	//FLAGS -> 00000100
//}
//
//void message::setSYN() {
//	//����SYNλΪ1
//	this->FLAGS = SYN;
//	//FLAGS -> 00000010
//}
//
//void message::setSYNACK() {
//	//����ACK��SYNλΪ1
//	this->FLAGS = SYN_ACK;
//	//FLAGS -> 00000110
//}
//
//void message::setFIN() {
//	//����FINλΪ1
//	this->FLAGS = FIN;
//	//FLAGS -> 00000001
//}
//
//void message::setFINACK() {
//	//����FIN,ACKλΪ1
//	this->FLAGS = FIN_ACK;
//	//FLAGS -> 00000101
//}

void message::setHEAD(int messeq, int fileSize, char* fileName) {
	//����HEADλΪ1
	this->FLAGS = HEAD; //FLAGS->00001000
	this->filelen = fileSize;
	this->messeq = messeq;
	memcpy(this->data, fileName, strlen(fileName) + 1);
}

//void message::setTAIL() {
//	//����TAILλΪ1
//	this->FLAGS = TAIL;
//	//FLAGS -> 00010000
//}

void message::fillData(int messeq, int size, char* data) {
	//���ļ������������ݰ�data����
	this->messeq = messeq;
	this->filelen = size;
	memcpy(this->data, data, size);
}

//����У���
uint16_t calChecksum(uint16_t* mes) {
	int size = MES_SIZE;	//�õ����ݵĴ�С(��λΪ�ֽ�)
	int count = size / 2 - 1;	//ÿ��ѭ������2�ֽ�
	uint16_t* buf = (uint16_t*)malloc(size);	//���ڱ������ݵĻ�����
	memset(buf, 0, size);
	memcpy(buf, mes, size); //����
	uint32_t sum = 0;
	while (count--) {
		sum += *buf; //�ۼ�
		buf++;
		//���sum�ĸ�16λ��Ϊ0����
		if (sum & 0xFFFF0000) {
			//�洢��16λ��
			sum &= 0xFFFF;
			sum++;
		}
	}
	return ~(sum & 0xFFFF);
}

//����У���
void message::setchecksum(uint16_t* mes) {
	this->checksum = calChecksum(mes); //��λȡ�����������
}

//�ж��ļ��Ƿ���
bool isCorrupt(message* mes) {
	if (calChecksum((uint16_t*)mes) == 0) {
		return false;
	}
	return true;
}

//��ӡ���ݰ���Ϣ
void printMessage(message* Mes) {
	cout << "���ݰ���С=" << Mes->filelen << "���أ���־λ=" << Mes->FLAGS << endl;
	cout << "�������=" << Mes->messeq << "ȷ�����=" << Mes->ackseq << endl;
	cout << "У���=" << Mes->checksum << endl;
}

//��ӡ�������ݰ���Ϣ
void printSendMessage(message* Mes) {
	cout << "�����Ͱ���Ϣ��" << endl;
	printMessage(Mes);
}

//��ӡ�������ݰ���Ϣ
void printRecvMessage(message* Mes) {
	cout << "�����հ���Ϣ��" << endl;
	printMessage(Mes);
}

//�жϷ������
bool has_seq1(message* Mes) {
	if (Mes->messeq == 1) {
		return true;
	}
	return false;
}

//�жϷ������
bool has_seq0(message* Mes) {
	if (Mes->messeq == 0) {
		return true;
	}
	return false;
}

//�ж�ACK���
bool isACK(message* Mes, int ack) {
	if (Mes->ackseq == ack && Mes->FLAGS == ACK) {
		return true;
	}
	return false;
}



//��ӡϵͳʱ��
void printTime() {
	cout << endl;
	stringstream ss;	//����һ��stringstream����ss�����ڽ�����ת��Ϊ�ַ���
	SYSTEMTIME sysTime = { 0 };	//�ṹ��SYSTEMTIME�����ڱ���ϵͳʱ��ĸ�����ɲ���
	ss.clear();
	ss.str("");	//���stringstream
	//��ȡ��ǰϵͳʱ�䣬���洢��sysTime�ṹ����
	GetSystemTime(&sysTime);
	//��ʱ��д��stringstream����ss
	ss << "[" << sysTime.wYear << "/" << sysTime.wMonth << "/" << sysTime.wDay
		<< " " << sysTime.wHour + 8 << ":" << sysTime.wMinute << ":"
		<< sysTime.wSecond << ":" << sysTime.wMilliseconds << "]";
	cout << "ʱ�䣺" << ss.str() << endl;
}

//�����Ϣ�ṹ
void message::clearMes() {
	this->FLAGS = this->sendIP = this->recvIP = this->sendPort = this->recvPort = 0;
	this->ackseq = this->messeq = this->filelen = this->checksum = 0;
	memset(data, 0, sizeof(data));
}