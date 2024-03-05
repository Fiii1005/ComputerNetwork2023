#include <fstream>
#include <Winsock2.h>
#include "message.h"
#include <Windows.h>
#include <random>
#include <iostream>

using namespace std;

//����dll��lib
#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE sizeof(message)
//ÿ����ʱʱ��=�����*��ʱ����(10����)
int DELAY_BASE = 10;
//������ ��Ϊ<0ʱ�����������
float LOSS_RATE = -1;
//��ʱ�� ��Ϊ<0ʱ�����������
float DELAY_RATE = -1;


SOCKET sersock;		//���Ͷ��׽���
sockaddr_in seraddr;	//�׽���IP�Ͷ˿ں�
sockaddr_in cliaddr;	//�ͻ��˵�IP�Ͷ˿ں�
int cliaddr_len = sizeof(sockaddr_in);	//�洢cliaddr�������Ĵ�С

//���ó�ʱʱ��
struct timeval tv;	//��������select�����ĳ�ʱʱ��
fd_set readfds;		//�ļ����������ϣ����ڼ����ļ���������״̬


default_random_engine randomGen;
uniform_real_distribution<float> randomNum(0.0, 1.0);  // ͨ����������ö���

void setTO() {
	FD_ZERO(&readfds);	//��ʼ������Ϊ��
	FD_SET(sersock, &readfds);	//�����׽���
	tv.tv_sec = 1;	//���ó�ʱʱ��Ϊ1s
}

//����ACK��
void sendACK(int seq) {
	message* sendMes = new message;
	sendMes->FLAGS = ACK;
	sendMes->ackseq = seq;
	sendMes->setchecksum((uint16_t*)sendMes);
	int err = 0;
	err = sendto(sersock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&cliaddr, cliaddr_len);
	if (err > 0) {
		cout << "����ACK_" << seq << "�ɹ���" << endl;
	}
	else {
		cout << "����ACK_" << seq << "ʧ�ܣ�" << endl;
	}
	delete sendMes;
}

//���ͱ�־λ��
void sendFLAGS(int f) {
	message* sendMes = new message();
	sendMes->FLAGS = f;
	int err = 0;
	err = sendto(sersock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&cliaddr, cliaddr_len);
	printTime();
	if (err > 0) {
		cout << "�ɹ����ͱ�־λ��";
		switch (f) {
		case ACK:
			cout << "ACK";
			break;
		case SYN:
			cout << "SYN";
			break;
		case SYN_ACK:
			cout << "SYN_ACK";
			break;
		case FIN:
			cout << "FIN";
			break;
		case FIN_ACK:
			cout << "FIN_ACK";
			break;
		case TAIL:
			cout << "TAIL";
			break;
		}
		cout << endl;
	}
	else {
		cout << "�������ݰ�ʧ�ܣ�������룺" << WSAGetLastError() << endl;
	}
	delete sendMes;
}

//�������ֽ�������
void connect() {
	int state = 0; //��ʶĿǰ���ֵ�״̬
	bool flag = true;  //�����Ƿ��������
	int res = 0;
	message* recvMes = new message;
	/*
	int WSAAPI recvfrom(
	  [in]                SOCKET   s,		//���׽���
	  [out]               char     *buf,	//�������ݵĻ�����
	  [in]                int      len,		//����������
	  [in]                int      flags,	//
	  [out]               sockaddr *from,	//sockaddr�ṹ���û��������ڷ���ʱ����Դ��ַ
	  [in, out, optional] int      *fromlen	//�洢sockaddr�������Ĵ�С
	);
	*/
	u_long mode = 1;
	int err = 0;
	while (flag) {
		switch (state) {
		case 0: //�ȴ��ͻ��˷���SYN���ݰ�״̬
			// ��������ģʽ
			setTO();
			err=select(sersock + 1, &readfds, NULL, NULL, NULL); //timeout����ΪNULL��ʾ�ȴ����޳�ʱ��
			if (FD_ISSET(sersock, &readfds)) { //����յ�����
				printTime();
				res = recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len);
				if (res > 0) {	//���յ�����
					if (recvMes->FLAGS == SYN) {
						cout << "�յ����Կͻ��˵��������󣡵�һ�����ֳɹ���" << endl;
						//�������˷���SYN_ACK�����ڶ�������
						sendFLAGS(SYN_ACK);
						/*
						int WSAAPI sendto(
						  [in] SOCKET         s,		//���ӵ��׽���
						  [in] const char     *buf,		//Ҫ��������ݵĻ�����ָ��
						  [in] int            len,		//bufָ������ݵĳ���
						  [in] int            flags,	//���÷�ʽ�ı�־
						  [in] const sockaddr *to,		//Ŀ���ַ
						  [in] int            tolen		//��ַ��С
						);
						*/
						state = 1; //ת״̬1
					}
					else {
						cout << "��һ���������ݰ���ƥ�䣡" << endl;
					}
				}
				else {
					cout << "�������ݲ���ȷ��" << WSAGetLastError() << endl;
				}
			}
			else {
				printTime();
				cout << "�������ݱ�����" << WSAGetLastError() << endl;
			}
			break;
		case 1:	//���տͻ��˵�ACK=1���ݰ�
			//select����ȷ��һ�������׽��ֵ�״̬�����ڱ�Ҫʱ�ȴ�ִ��ͬ��I/O��
			//�ж��Ƿ�ʱ
			setTO();
			select(sersock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(sersock, &readfds)) {
				//����ڳ�ʱʱ�����յ�����
				if (recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len) > 0) {
					printTime();
					if (recvMes->FLAGS == ACK) {
						cout << "�ͻ��˷���ACK�������������ֳɹ���" << endl;
						cout << "����������������������������������������������������������������" << endl;
						flag = false;
					}
					else {
						cout << "�ͻ��˷��ͷ�ACK����" << endl;
					}
				}
			}
			else { //���������ʱ
				printTime();
				cout << "��ʱδ�յ��ͻ���ACK���ط�SYN_ACK����" << endl;
				sendFLAGS(SYN_ACK);
			}
			break;
		}
	}
	delete recvMes;
}

//�Ͽ�����
void disconnect() {
	int state = 0; //��ʶĿǰ����״̬
	bool flag = true; //Ϊtrueʱû�л��ֽ���
	int res = 0;
	message* recvMes = new message();

	while (flag) {
		switch (state) {
		case 0:
			printTime();
			cout << "�ͻ��˷��������������" << endl;
			//��ͻ��˷���ACK��
			sendFLAGS(ACK);
			state = 1;
			break;
		case 1:
			printTime();
			cout << "��ʼ�����λ��֣�����FINACK����" << endl;
			sendFLAGS(FIN_ACK);
			state = 2;
			break;
		case 2:
			//�ж��Ƿ�ʱ
			setTO();
			select(sersock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(sersock, &readfds)) {
				//����ڳ�ʱʱ�����յ�����
				printTime();
				if (recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len) > 0) {
					if (recvMes->FLAGS == ACK) {
						cout << "�ͻ��˷���ACK�������Ĵλ��ֳɹ���" << endl;
						cout << "����������������������������������������������������������������" << endl;
						flag = false;
					}
					else {
						cout << "�ͻ��˷��ͷ�ACK����" << endl;
					}
				}
			}
			else { //���������ʱ
				printTime();
				cout << "��ʱδ�յ��ͻ���ACK���ط�FIN_ACK����" << endl;
				sendFLAGS(FIN_ACK);
			}
			break;
		}
	}
	delete recvMes;
}

int fileSize = 0; //���ļ�����
char* fileName;
char* fileBuffer;
int packetnum;	//ĳ���ļ���Ҫ�����ݰ�����
unsigned int recvSize; //�ۼ��յ����ļ�λ�ã�����д�뻺������

//�����ļ�
void saveFile() {
	string filePath = "C:/Users/10141/Documents/Fi/2023Fall/7 ComputerNetwork/labs/3/3-1/save/";
	for (int i = 0; fileName[i]; i++) {
		filePath += fileName[i];
	}
	cout << "�ļ�·��Ϊ��" << filePath << endl;
	/*ofstream fout;
	fout.open(filePath, ios::app);
	for (int i = 0; i < fileSize; i++) {
		fout << fileBuffer[i];
	}*/
	ofstream fout(filePath, ios::binary | ios::out);
	fout.write(fileBuffer, fileSize); // ���ﻹ��size,���ʹ��string.data��c_str�Ļ�ͼƬ����ʾ�������������
	printTime();
	cout << "��ǰ�ļ�����ɹ���" << endl;
	fout.close();
}


//�����ļ�
void receiveFile() {
	bool flag = true;
	int state = 0;
	int res = 0;	//���շ���
	int expectedseqnum = 0; //�ڴ����յ���һ���������
	

	message* recvMes = new message();


	while (flag) {	//ѭ�������ļ����ݰ�
		switch (state) {
		case 0:	//�ȴ����ݰ�ͷ�ļ�״̬
			res = recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len);
			if (res > 0) {
				if (randomNum(randomGen) <= LOSS_RATE) {//��������
					printTime();
					cout << "��������" << endl;
				}
				else {
					if (randomNum(randomGen) <= DELAY_RATE) {//��ʱ
						int time = DELAY_BASE * randomNum(randomGen);
						Sleep(time);
						printTime();
						cout << "��ʱ" << time << "����" << endl;
					}
					printTime();
					printRecvMessage(recvMes);
					if (recvMes->FLAGS == FIN) {
						flag = false;
						disconnect();
					}
					else if (isCorrupt(recvMes) || recvMes->messeq != 0) {
						cout << "��ǰ�ļ��ѽ�����ϣ��ȴ�������һ�ļ�" << endl;
						sendACK(expectedseqnum - 1);
					}
					else if (!isCorrupt(recvMes) && recvMes->messeq == 0) {
						if (recvMes->FLAGS == HEAD) {
							expectedseqnum = 1;
							cout << "�������ݰ�ͷ�ɹ���" << endl;
							//��ʼ�������ļ�������
							recvSize = 0;
							fileSize = recvMes->filelen;
							fileBuffer = new char[fileSize];
							fileName = new char[128];
							memcpy(fileName, recvMes->data, strlen(recvMes->data) + 1);
							//�����ļ�������
							packetnum = fileSize % 1024 ? fileSize / 1024 + 1 : fileSize / 1024;
							cout << "��ʼ�������Է��Ͷ˵��ļ����ļ���Ϊ��" << fileName << endl;
							cout << "�ļ���СΪ��" << fileSize << "���أ��ܹ���Ҫ����" << packetnum << "�����ݰ�" << endl;
							cout << "�ȴ������ļ����ݰ���������" << endl;
							//����ACK_0���ݰ�
							sendACK(0);
							state = 1;
						}
						else {
							cout << "�յ������ݰ������ļ�ͷ���ȴ����Ͷ��ش�����" << endl;
						}
					}
				}
			}
			break;
		case 1:
			res = recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len);
			if (res > 0) {
				if (randomNum(randomGen) <= LOSS_RATE) {//��������
					printTime();
					cout << "��������" << endl;
				}
				else {
					if (randomNum(randomGen) <= DELAY_RATE) {//��ʱ
						int time = DELAY_BASE * randomNum(randomGen);
						Sleep(time);
						printTime();
						cout << "��ʱ" << time << "����" << endl;
					}
					printTime();
					printRecvMessage(recvMes);
					//�ۻ�ȷ��
					if (!isCorrupt(recvMes) && recvMes->messeq == expectedseqnum) {
						//�յ���Ŀ��İ�
						memcpy(fileBuffer + recvSize, recvMes->data, recvMes->filelen);
						recvSize += recvMes->filelen;
						sendACK(expectedseqnum);
						expectedseqnum++;
						if (recvMes->FLAGS == TAIL) {
							//��������һ����������״̬0��Ҳ���ǵȴ�ͷ�ļ����߶Ͽ�����
							state = 0;
							cout << "��ǰ�ļ�������ϣ�" << endl;
							cout << "������������������������������������" << endl;
							//�����ļ�
							saveFile();
						}
						else {
							cout << endl << "�ļ������ճɹ���" << endl;
						}
					}
					else {
						cout << endl << "���𻵻���Ŵ���" << endl;
						sendACK(expectedseqnum - 1);
					}
				}
			}
			else {
				cout << "���ܰ�����" << endl;
			}
			break;
		}
	}
	delete recvMes;
}

int main() {
	//����socket��
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "socket��ʼ��ʧ��" << endl;
		return 0;
	}
	//����socket
	sersock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//ipv4��ַ��ʹ��UDP���ݱ��׽��֣���ָ��Э��
	if (sersock == INVALID_SOCKET) {
		cout << "socket����ʧ��" << endl;
		return -1;
	}

	cout << "�������˳�ʼ�����" << endl;
	cout << "IP��ַ��127.0.0.1 �˿ںţ�8000" << endl;

	//��ʼ����ַ
	//Ĭ��IP
	//�����׽��ֵ�IP�Ͷ˿�
	memset(&seraddr, 0, sizeof(sockaddr_in)); //���addr�ڴ沿�ֵ�ֵ
	seraddr.sin_family = AF_INET;
	//ʹ��htons��u_short������ת��ΪIP�����ֽ�˳�򣨴�ˣ�
	string serIP = "127.0.0.1";
	int port = 8000;
	/*cout << "������IP��ַ��" << endl;
	cin >> serIP;
	cout << "������˿ںţ�" << endl;
	cin >> port;*/
	char* portc = new char[20];
	memcpy(portc, serIP.c_str(),20);

	seraddr.sin_port = htons(port);
	//ʹ��inet_addr������IPv4��ַ���ַ���ת��Ϊs_addr�ṹ
	seraddr.sin_addr.s_addr = inet_addr(portc);
	//���׽��ְ󶨵�����IP�Ͷ˿�
	if (bind(sersock, (SOCKADDR*)&seraddr, sizeof(SOCKADDR))==-1) {
		//����󶨷�������
		cout << "�׽��ְ󶨷�������" << WSAGetLastError() << endl;
		return 0;
	}

	cout << "�����ö����ʣ�0~1������Ϊ<0ʱ���ᷢ����������" << endl;
	cin >> LOSS_RATE;
	cout << "��������ʱ�ʣ�0~1������Ϊ<0ʱ���ᷢ����ʱ����" << endl;
	cin >> DELAY_RATE;
	/*cout << "�����ó�ʱʱ�䣨���룩��" << endl;
	cin >> DELAY_BASE;*/

	cout << "�������������ɹ����ȴ��ͻ��˽�������" << endl;

	connect();

	receiveFile();

	//�ر�socket
	int res = closesocket(sersock);
	if (res == SOCKET_ERROR) {
		cout << "�ر��׽��ִ���" << WSAGetLastError() << endl;
		return 1;
	}
	printTime();
	cout << "�����˳�����" << endl;
	WSACleanup();

	delete fileBuffer;
	delete fileName;
	delete []portc;

	return 0;
}