#include <fstream>
#include <winsock2.h>
#include <string>
#include <thread>
#include <ctime>
#include <iostream>
#include <mutex>
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
std::mutex mylock;

//�������ڴ�С,Ĭ��4
int windowsize = 4;

//��ʱ�ȴ�ʱ��,Ĭ��250����
int waittime = 250;

//���ó�ʱʱ��
float timeout = 200;
//void setTimeout() {
//	timeout = timeout / 1000 * CLOCKS_PER_SEC;
//}
//��ʱ��
int timer = 0;

//���ó�ʱʱ��
struct timeval tv;	//��������select�����ĳ�ʱʱ��
fd_set readfds;		//�ļ����������ϣ����ڼ����ļ���������״̬

void setTO() {
	FD_ZERO(&readfds);	//��ʼ������Ϊ��
	FD_SET(clisock, &readfds);	//�����׽���
	tv.tv_sec = 0;	//���ó�ʱʱ��Ϊ1s
	tv.tv_usec = waittime;
}

//class WindowBuffer {
//public:
//	static WindowBuffer* baseP;
//	static WindowBuffer* endP;
//	message* thisMes;
//	message* nextMes;
//	WindowBuffer() {
//		thisMes = new message();
//		nextMes = thisMes;
//	}
//};
//WindowBuffer* WindowBuffer::baseP = NULL;
//WindowBuffer* WindowBuffer::endP = NULL;


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

int sendbase = 0;
int nextseqnum = 0; //��һ��Ҫ���Ͱ������
message** sendMes;
message* recvMes;
int lostnum = 0;//������
int allsendnum = 0; //���з��͵İ���
clock_t StartTimer; //��ʱ��
bool EndTimer = false;
bool needResend = false; //�����ж��Ƿ��յ������ACK������ǣ��ش�

//�ش�����
void resendMes() {
	cout << "���������ش������ڰ���������" << endl;
	for (int i = sendbase; i < nextseqnum; i++) {
		int index = i % windowsize;
		sendto(clisock, (char*)sendMes[index], BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
		printTime();
		cout << "���ش����ݰ���";
		printSendMessage(sendMes[index]);
		lostnum++;
		allsendnum++;
	}
}

void recvThread() {
	while (sendbase <= packetNum) {
		if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
			printTime();
			if (recvMes->FLAGS == ACK && !isCorrupt(recvMes)) {
				cout << "�յ�ACK��" << recvMes->ackseq << endl;
				if ((recvMes->ackseq + 1) <= (sendbase)) {
					cout << "���ڴ�Сδ�ı䣬�����ش�";
					cout << recvMes->ackseq << "::" << sendbase << endl;
					{
						std::lock_guard<std::mutex> mylockguard(mylock);
						needResend = true;
					}
				}
				else {
					{
						std::lock_guard<std::mutex> mylockguard(mylock);
						sendbase = recvMes->ackseq + 1;
						cout << "���ڱ߽�base��Ϊ��" << sendbase << endl;
						if (sendbase == nextseqnum) {
							//KillTimer(NULL, timer);
							EndTimer = true;
							cout << "��������ֹͣ��ʱ��������" << endl;
						}
						else {
							//timer = SetTimer(NULL, 0, timeout, (TIMERPROC)resendMes);
							EndTimer = false;
							StartTimer = clock();
						}
					}
				}
			}
			else {
				cout << "���յİ����𻵻���ACK��" << endl;
			}
		}
	}
}

////���ó�ʱ
//atomic<bool> StartTimer(false);
//atomic<bool> StopTimer(false);
//atomic<bool> EndTimer(false);
//
//void TimerThread() {
//	while (true) {
//		if (StartTimer) {
//			std::this_thread::sleep_for(std::chrono::milliseconds((int)timeout));
//			if (!StopTimer) {
//				resendMes();
//			}
//		}
//		if (EndTimer) {
//			break;
//		}
//	}
//}

//�����ļ�
void sendFile() {
	sendMes = new message * [windowsize];
	sendbase = 0;
	nextseqnum = 0; //��һ��Ҫ���Ͱ������
	needResend = false;

	//���ڼ��㶪����
	allsendnum = 0; //���з��͵İ���
	lostnum = 0;//������
	//�ȷ���һ����¼�ļ��������ݰ���������HEAD��־λΪ1����ʾ��ʼ�ļ�����
	for (int i = 0; i < windowsize; i++) {
		sendMes[i] = new message();
	}
	recvMes = new message();
	printTime();
	clock_t start = clock();
	cout << "�����ļ�ͷ���ݰ���������" << endl;
	char* fileNamec = new char[128];
	strcpy(fileNamec, fileName.c_str());
	sendMes[nextseqnum-sendbase]->setHEAD(0, fileSize, fileNamec);
	sendMes[nextseqnum - sendbase]->setchecksum((uint16_t*)sendMes[nextseqnum - sendbase]);
	printSendMessage(sendMes[nextseqnum - sendbase]);
	sendto(clisock, (char*)sendMes[nextseqnum - sendbase], BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
	//// ������ʱ���߳�
	//StartTimer = true;
	//EndTimer = false;
	//StopTimer = false;
	//thread timerThread(TimerThread);
	StartTimer = clock();
	EndTimer = false;

	allsendnum++;
	nextseqnum++;
	//����������Ϣ���߳�
	thread rThread(recvThread);

	while (sendbase <= packetNum) { //��baseС�ڷ��Ͱ�����ʱ����������
		clock_t StopTimer = clock();
		double intival = (StopTimer - StartTimer) * 1000 / CLOCKS_PER_SEC;
		cout << "��ʱʱ��Ϊ��" << intival << endl;
		if ( (intival > timeout && !EndTimer) || needResend) { //�����ʱ���߽��մ���ACK���ش�
			if (intival > timeout && !EndTimer) {
				cout << "���ó�ʱʱ��Ϊ��" << timeout << endl;
				cout << "��ʱʱ��Ϊ��" << intival << endl;
			}
			{
				std::lock_guard<std::mutex> mylockguard(mylock);
				needResend = false;
				resendMes();
				StartTimer = clock();
				EndTimer = false;
			}
		}
		if (nextseqnum < sendbase + windowsize && nextseqnum <= packetNum) { //�������û����û�з������
			printTime();
			{
				std::lock_guard<std::mutex> mylockguard(mylock);
				cout << "���ڷ��͵�" << nextseqnum << "�����ݰ���" << endl;
				cout << "baseΪ��" << sendbase << endl;
				int index = nextseqnum % windowsize;
				sendMes[index]->clearMes();
				if (nextseqnum == packetNum) { //��������һ���ļ��������ļ�β
					sendMes[index]->FLAGS = TAIL;
				}
				sendMes[index]->fillData(nextseqnum, (fileSize - (nextseqnum - 1) * 1024) > 1024 ? 1024 : fileSize - (nextseqnum - 1) * 1024, fileBuffer + (nextseqnum - 1) * 1024);
				sendMes[index]->setchecksum((uint16_t*)sendMes[index]);
				printSendMessage(sendMes[index]);
				res = sendto(clisock, (char*)sendMes[index], BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
				if (sendbase == nextseqnum) {
					EndTimer = false;
					StartTimer = clock();
				}
				if (res > 0) {
					allsendnum++;
					cout << "���ݰ����ͳɹ���" << endl;
					//printSendMessage(sendMes);
					nextseqnum++;
				}
				else {
					cout << "���ݰ�����ʧ�ܣ�" << endl;
				}
			}
		}
		else {
			cout << "������������ļ��ѷ�����ϣ��ȴ�ȷ�ϡ�" << endl;
			Sleep(20);
		}
	}
	EndTimer = true;
	//timerThread.join();
	rThread.join();
	printTime();
	cout << "��ǰ�ļ�������ϡ�" << endl;
	clock_t end = clock();
	cout << "�ܴ���ʱ��Ϊ��" << (end - start) / CLOCKS_PER_SEC << "��" << endl;
	cout << "������Ϊ��" << (float)fileSize / ((end - start) * 1000 / CLOCKS_PER_SEC) << "����/����" << endl;
	cout << "���󣨶���+��ʱ����Ϊ��" << (float)lostnum / allsendnum << endl;

	//�����ڴ�
	for (int i = 0; i < windowsize; i++) {
		delete sendMes[i];
	}
	delete[] sendMes;
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

	int portnum = 8000;
	cout << "��ѡ��ʹ��·����ѡ0��ʹ���Լ�д�Ķ���ѡ1" << endl;
	int portchoose = 1;
	cin >> portchoose;
	if (portchoose == 0) {
		portnum = 8001;
	}

	//��ʼ����ַ
	//Ĭ��IP
	//�����׽��ֵ�IP�Ͷ˿�
	memset(&seraddr, 0, sizeof(sockaddr_in)); //���addr�ڴ沿�ֵ�ֵ
	seraddr.sin_family = AF_INET;
	//ʹ��htons��u_short������ת��ΪIP�����ֽ�˳�򣨴�ˣ�
	seraddr.sin_port = htons(portnum);
	//ʹ��inet_addr������IPv4��ַ���ַ���ת��Ϊs_addr�ṹ
	seraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//·����IP��127.0.0.1 �˿ڣ�8001
	//������IP��127.0.0.1 �˿ڣ�8000


	/*cout << "�����ó�ʱʱ�䣨���룩��" << endl;
	cin >> timeout;*/
	//setTimeout();

	cout << "�����û������ڴ�С��4~32����" << endl;
	cin >> windowsize;
	cout << "�������ڴ�СΪ��" << windowsize << endl;

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