#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <string>
#include "message.h"
using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE sizeof(message)

SOCKET clisock;
SOCKADDR_IN seraddr;
SOCKADDR_IN cliaddr;
int res = 0; //�����ļ��Ľ��
int seraddr_len = sizeof(sockaddr_in);	//�洢sockaddr�������Ĵ�С
int cliaddr_len = sizeof(sockaddr_in);

//��ʱ�ȴ�ʱ��,Ĭ��250����
int waittime = 250;


//���ó�ʱʱ��
struct timeval tv;	//��������select�����ĳ�ʱʱ��
fd_set readfds;		//�ļ����������ϣ����ڼ����ļ���������״̬

void setTO() {
	FD_ZERO(&readfds);	//��ʼ������Ϊ��
	FD_SET(clisock, &readfds);	//�����׽���
	tv.tv_sec = 0;	//���ó�ʱʱ��Ϊ1s
	tv.tv_usec = waittime;
}

//����ACK��
void sendACK(int seq) {
	message* sendMes = new message();
	sendMes->FLAGS = ACK;
	sendMes->ackseq = seq;
	sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
	delete sendMes;
}

//���ͱ�־λ��
void sendFLAGS(int f) {
	message* sendMes = new message();
	sendMes->FLAGS = f;
	res=sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
	if (res > 0) {
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


void connect() {
	int state = 0;
	bool flag = true;
	message* recvMes = new message();
	while (flag) {
		switch (state) {
		case 0: //��������˷���SYNλ
			printTime();
			cout << "��ʼ��һ�����֣���������˷���SYN����" << endl;
			sendFLAGS(SYN);
			state = 1;
			break;
		case 1: //�ڳ�ʱʱ�����յ�SYN_ACK����������ACK
			//�ж��Ƿ�ʱ
			setTO();
			select(clisock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(clisock, &readfds)) {
				//����ڳ�ʱʱ�����յ�����
				if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
					printTime();
					if (recvMes->FLAGS == SYN_ACK) {
						cout << "�ͻ��˷���SYN_ACK��������ACK���ݰ���" << endl;
						sendFLAGS(ACK);
						cout << "��������˽������ӣ�" << endl;
						cout << "����������������������������������������" << endl;
						flag = false; //�������
					}
					else {
						cout << "�������˷��ͷ�SYN_ACK����" << endl;
					}
				}
			}
			else { //���������ʱ
				printTime();
				cout << "��ʱδ�յ���������SYN_ACK���ط�SYN����" << endl;
				sendFLAGS(SYN);
			}
			break;
		}
	}
	delete recvMes;
}

//�˳�����
void disconnect() {
	int state = 0;
	bool flag = true;

	message* recvMes = new message();

	while (flag) {
		switch (state) {
		case 0: //��һ�λ��֣�����FIN��
			printTime();
			cout << "�������ӣ�����FIN����" << endl;
			sendFLAGS(FIN);
			state = 1;
			break;
		case 1:
			//�ж��Ƿ�ʱ
			setTO();
			select(clisock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(clisock, &readfds)) {
				//����ڳ�ʱʱ�����յ�����
				printTime();
				if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
					if (recvMes->FLAGS == ACK) {
						cout << "�������˷���ACK�����ڶ��λ��ֳɹ���" << endl;
						state = 2;
					}
					else {
						cout << "�ͻ��˷��ͷ�ACK����" << endl;
					}
				}
			}
			else { //���������ʱ
				printTime();
				cout << "��ʱδ�յ��ͻ���ACK���ط�FIN����" << endl;
				sendFLAGS(FIN);
			}
			break;
		case 2:
			res = recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len);
			if (res >= 0) {
				if (recvMes->FLAGS == FIN_ACK) {
					printTime();
					cout << "�յ������Է������˵�FIN_ACK���������λ��ֳɹ���" << endl;
					state = 3;
				}
				else {
					printTime();
					cout << "�������˷��ͷ�FIN_ACK����" << endl;
				}
			}
			break;
		case 3: //��������˷���ACK��
			printTime();
			cout << "��������˷���ACK�����������ӣ�" << endl;
			sendFLAGS(ACK);
			flag = false;
			break;
		}
	}
	delete recvMes;
}

int fileSize; //�ļ���С
string fileName; //�ļ�����
string filePath; //�ļ�·��
char* fileBuffer; //�ļ�������
int packetNum; //���ļ���

//��ȡ�ļ�
bool readFile() {
	//cout << "������Ҫ������ļ�·��������·��ʹ��/����" << endl;
	//cin.ignore();
	//getline(cin, filePath);
	cout << "������Ҫ������ļ�����������׺����" << endl;
	cin.ignore();
	getline(cin, fileName);
	//C:/Users/10141/Documents/Fi/2023Fall/7 ComputerNetwork/labs/3/3-1/send/
	filePath = "C:/Users/10141/Documents/Fi/2023Fall/7 ComputerNetwork/labs/3/ʵ��3�����ļ���·��������/�����ļ�/" + fileName;
	// fileName = "test.txt";
	ifstream f(filePath, ifstream::in | ios::binary);	//�Զ����Ʒ�ʽ��
	if (!f.is_open()) {
		cout << "�ļ��޷��򿪣�" << endl;
		return false;
	}
	f.seekg(0, std::ios_base::end);//���ļ���ָ�붨λ������ĩβ
	fileSize = f.tellg();
	packetNum = fileSize % 1024 ? fileSize / 1024 + 1 : fileSize / 1024;
	cout << "�ļ���СΪ��" << fileSize << "���أ��ܹ�Ҫ����" << packetNum << "�����ݰ�" << endl;
	f.seekg(0, std::ios_base::beg); //���ļ���ָ�����¶�λ�����Ŀ�ʼ
	fileBuffer = new char[fileSize];
	if (fileBuffer == NULL) {
		cout << "�ڴ����ʧ�ܣ�" << endl;
		f.close();
		return false;
	}
	f.read(fileBuffer, fileSize);
	f.close();
	return true;
}

//�����ļ�
void sendFile() {
	int allsendnum = 0; //���з��͵İ���
	int lostnum = 0;//������
	//�ȷ���һ����¼�ļ��������ݰ���������HEAD��־λΪ1����ʾ��ʼ�ļ�����
	message* sendMes = new message;
	message* recvMes = new message;
	printTime();
	clock_t start = clock();
	cout << "�����ļ�ͷ���ݰ���������" << endl;
	char* fileNamec = new char[128];
	strcpy(fileNamec, fileName.c_str());
	sendMes->setHEAD(1, fileSize, fileNamec);
	sendMes->setchecksum((uint16_t*)sendMes);
	printSendMessage(sendMes);
	sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
	int sendnum = 0; //�ѷ����ļ�������
	int state = 0;
	allsendnum++;
	bool flag = true;
	while (flag) {
		switch (state) {
		case 0:
			//�ж��Ƿ�ʱ
			setTO();
			select(clisock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(clisock, &readfds)) {
				//����ڳ�ʱʱ�����յ�����
				printTime();
				if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
					if (isACK(recvMes, 1)&&!isCorrupt(recvMes)) {
						cout << "�յ�ACK_1" << endl;
						if (sendnum == packetNum) {
							state = 4;
						}
						else {
							state = 1;
						}
					}
					else {
						cout << "���յİ����𻵻���ACK_1����" << endl;
					}
				}
				else {
					cout << "����ACK������" << WSAGetLastError() << endl;
				}
			}
			else { //���������ʱ
				printTime();
				cout << "��ʱδ�յ�ACK���ط����ݰ���" << endl;
				allsendnum++;
				lostnum++;
				sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
			}
			break;
		case 1:
			//������һ���ݰ�
			printTime();
			cout << "���ڷ��͵�" << sendnum << "�����ݰ���" << endl;
			sendMes->clearMes();
			//��������һ����
			if (sendnum == packetNum-1) {
				sendMes->FLAGS=TAIL;
			}
			sendMes->messeq = 0;
			sendMes->fillData(0, (fileSize - sendnum * 1024) > 1024 ? 1024 : fileSize - sendnum * 1024, fileBuffer + sendnum * 1024);
			sendMes->setchecksum((uint16_t*)sendMes);
			cout << "testУ���Ϊ��" << sendMes->checksum << endl << endl;
			res = sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
			if (res > 0) {
				allsendnum++;
				cout << "���ݰ����ͳɹ���" << endl;
				printSendMessage(sendMes);
				sendnum++;
				state = 2;
			}
			else {
				cout << "���ݰ�����ʧ�ܣ�" << endl;
			}
			break;
		case 2:
			//�ж��Ƿ�ʱ
			setTO();
			select(clisock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(clisock, &readfds)) {
				//����ڳ�ʱʱ�����յ�����
				printTime();
				if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
					if (isACK(recvMes, 0)&&!isCorrupt(recvMes)) {
						cout << "�յ�ACK_0" << endl;
						if (sendnum == packetNum) {
							state = 4;
						}
						else {
							state = 3;
						}
					}
					else {
						cout << "���յİ����𻵻���ACK_0����" << endl;
					}
				}
				else {
					cout << "����ACK������" << WSAGetLastError() << endl;
				}
			}
			else { //���������ʱ
				printTime();
				lostnum++;
				allsendnum++;
				cout << "��ʱδ�յ�ACK���ط����ݰ���" << endl;
				sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
			}
			break;
		case 3:
			//������һ���ݰ�
			printTime();
			cout << "���ڷ��͵�" << sendnum << "�����ݰ���" << endl;
			sendMes->clearMes();
			//��������һ����
			if (sendnum == packetNum - 1) {
				sendMes->FLAGS = TAIL;
			}
			sendMes->fillData(1, (fileSize - sendnum * 1024) > 1024 ? 1024 : fileSize - sendnum * 1024, fileBuffer + sendnum * 1024);
			sendMes->setchecksum((uint16_t*)sendMes);
			cout << "testУ���Ϊ��" << sendMes->checksum << endl << endl;
			res = sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
			if (res > 0) {
				cout << "���ݰ����ͳɹ���" << endl;
				allsendnum++;
				printSendMessage(sendMes);
				sendnum++;
				state = 0;
			}
			else {
				cout << "���ݰ�����ʧ�ܣ�" << endl;
			}
			break;
		case 4: //��ǰ�ļ��Ѿ�������
			flag = false;
			printTime();
			cout << "��ǰ�ļ�������ϡ�" << endl;
			clock_t end = clock();
			cout << "�ܴ���ʱ��Ϊ��" << (end - start)/CLOCKS_PER_SEC << "��" << endl;
			cout << "������Ϊ��" << (float)fileSize / ((end - start)*1000 / CLOCKS_PER_SEC) << "����/����" << endl;
			cout << "���󣨶���+��ʱ����Ϊ��" << (float)lostnum / allsendnum << endl;
			break;
		}
	}
	delete sendMes;
	delete recvMes;
	delete []fileNamec;
}

int main() {
	//��ʼ��socket����
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "socket��ʼ��ʧ��" << endl;
		return 0;
	}
	//����socket
	clisock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//ipv4��ַ��ʹ��UDP���ݱ��׽��֣���ָ��Э��
	if (clisock == INVALID_SOCKET) {
		cout << "socket����ʧ��" << endl;
		return -1;
	}

	//��ʼ����ַ
	//Ĭ��IP
	//�����׽��ֵ�IP�Ͷ˿�
	memset(&seraddr, 0, sizeof(sockaddr_in)); //���addr�ڴ沿�ֵ�ֵ
	seraddr.sin_family = AF_INET;
	//ʹ��htons��u_short������ת��ΪIP�����ֽ�˳�򣨴�ˣ�
	seraddr.sin_port = htons(8000);
	//ʹ��inet_addr������IPv4��ַ���ַ���ת��Ϊs_addr�ṹ
	seraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//·����IP��127.0.0.1 �˿ڣ�8001
	//������IP��127.0.0.1 �˿ڣ�8000


	cout << "���������ʱ�ȴ�ʱ�䣨���룩��" << endl;
	cin >> waittime;

	cout << "�ͻ��˳�ʼ�����" << endl;

	connect();
	while (true) {
		cout << "�Ƿ�Ҫ�����ļ����ǣ�0/�˳���1" << endl;
		int order = -1;
		cin >> order;
		if (order == 1) {
			break;
		}
		else if (order == 0) {
			if (readFile()) {
				sendFile();
			}
		}
		else {
			cout << "ָ�����" << endl;
		}
	}
	//�Ͽ�����
	disconnect();
	
	res = closesocket(clisock);
	if (res == SOCKET_ERROR) {
		cout<< "�ر��׽��ִ���" << WSAGetLastError() << endl;
		return 1;
	}

	printTime();
	cout << "�����˳�����" << endl;
	WSACleanup();

	delete fileBuffer;

	
	return 0;
}