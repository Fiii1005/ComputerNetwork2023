#include <iostream>
#include <Winsock2.h>
#include <string>
using namespace std;

//����dll��lib
#pragma comment(lib, "ws2_32.lib")

//WSA������ʼ��
//���������
//��������
//-�����̣߳��ͻ��˼���
//-�����ͻ��˴���ȫ�ֽṹ������
//---��ÿ���ͻ���һ���߳�
//---��ÿ�յ�һ����Ϣ�ͽ��й㲥
//���߳��ж��Ƿ��˳�����
//�ر��߳�
//�رշ�������
//����WSA����

SOCKET ser_sock;					//���������׽���
sockaddr_in ser_addr;				//�׽���IP�Ͷ˿ں�
int saddr_len = sizeof(sockaddr_in);//sockaddr_in�ĳ���
bool bExit = false;					//�ж��Ƿ��˳�

//�洢�ͻ�����Ϣ�Ľṹ
int cnum = 0;		//�ͻ��˸���
struct client {
	int cid=-1;			//�ͻ��˱��
	SOCKET cli_sock;	//�ͻ����׽���
	HANDLE* cthread;	//�ͻ����߳�
}cli_inf[20];
//�㲥�ͻ��˺���
void bc(int cid, char* send_buf) {
	for (int i = 0; i < cnum; i++) {
		if (i == cid) {
			continue;
		}
		send(cli_inf[i].cli_sock, send_buf, 100, 0);
	}
	return;
}

//���տͻ�����Ϣ�̺߳���
DWORD WINAPI ClientThread(LPVOID cc) {
	//��ͻ���ͨѶ�����ղ�������Ϣ
	int cid = (int)cc;
	cout << "�����ͻ���" << cid << "���������ҡ���" << endl;
	//��������
	char send_buf[100] = { 0 };
	sprintf_s(send_buf,100, "�ͻ��� %d �ɹ����������ң�\n", cid);
	bc(-1, send_buf);	//�㲥
	//ѭ�����տͻ�������
	while (true) {
		char recv_buf[100] = { 0 };
		int ret = 100;	//��ʾrecv()��������ֵ
		ret = recv(cli_inf[cid].cli_sock, recv_buf, 100, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == 10053) {
				break;
			}
			cout << "�������ͻ���" << cid << "���մ���"<<WSAGetLastError()<<"������" << endl;
			break;
		}
		if (!strcmp(recv_buf,"exit")) {
			cout << "### �ͻ���" << cid << "���˳������� ###" << endl;
			memset(send_buf, '0', sizeof(send_buf));	//��շ��ͻ�����
			sprintf_s(send_buf,100, "### �ͻ��� %d ���˳������ң�### \n", cid);
			bc(cid, send_buf);
			break;
		}
		memset(send_buf, '0', sizeof(send_buf));
		sprintf_s(send_buf,100, "�ͻ��� %d : %s \n", cid, recv_buf);
		cout << send_buf;
		bc(cid, send_buf);
	}
	return 0;
}

//���տͻ����߳�
DWORD WINAPI ReceiveThread(LPVOID num) {
	//�߳�ѭ�����տͻ��˵�����
	while (true) {
		client* c = &cli_inf[cnum];
		sockaddr_in addrClient;
		//������clientͨѶ��socket
		c->cli_sock = accept(ser_sock, (SOCKADDR*)&addrClient, &saddr_len);
		c->cid = cnum;
		if (c->cli_sock != INVALID_SOCKET) {
			//������صĲ�����Ч�׽���
			//�����̣߳����ոÿͻ��˵���Ϣ��ת��
			HANDLE cThread = CreateThread(NULL, 0, ClientThread, (LPVOID)c->cid, 0, NULL);
			c->cthread = &cThread;
		}
		cnum++;
	}
	return 0;
}




int main() {
	//��ʼ��Winsock
	//����wsadata�Ķ���
	WSADATA wsaData;
	//���س�ʼ�����
	int wsaRe;
	wsaRe = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaRe != 0) {
		cout << "������WSA������ʼ��ʧ�ܣ�����" << endl;
		return 0;
	}

	cout << "���������������ˡ�������" << endl;
	//�����׽���
	ser_sock = socket(
		AF_INET,		//ʹ��IPv4��ַ
		SOCK_STREAM,	//ʹ����ʽ�׽���
		IPPROTO_TCP		//ʹ��TCPЭ��
	);
	//�����׽��ֵ�IP�Ͷ˿�
	memset(&ser_addr, 0, sizeof(sockaddr_in)); //���addr�ڴ沿�ֵ�ֵ
	ser_addr.sin_family = AF_INET;
	//ʹ��htons��u_short������ת��ΪTCP/IP�����ֽ�˳�򣨴�ˣ�
	ser_addr.sin_port = htons(8000);
	//ʹ��inet_addr������IPv4��ַ���ַ���ת��Ϊs_addr�ṹ
	ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//���׽��ְ󶨵�����IP�Ͷ˿�
	if (bind(ser_sock, (SOCKADDR*)&ser_addr, saddr_len)) {
		//����󶨷�������
		cout << "�׽��ְ󶨷�������"<< WSAGetLastError() << endl;
		return 0;
	}

	//��������
	if (listen(ser_sock, 20) != 0) {
		cout << "��������" << endl;
		return 0;
	}
	cout << "�������������������ڼ�����������" << endl;

	//���տͻ��˵����ӵ��߳�
	HANDLE rthread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)&cnum, 0, NULL);

	//��ѭ���ж����߳��Ƿ��˳�
	while (true) {
		string command = "0";
		getline(cin,command);
		if (command == "exit") {
			//�㲥���ͻ��˱�ʾ����������ֹͣ����
			char main_send[100] = { 0 };
			sprintf_s(main_send, 100, "exit");
			bc(-1, main_send);
			break;
		}
	}
	//�˳���������ָ��
	//�ر����д������߳�
	for (int i = 0; i < cnum; i++) {
		CloseHandle(*cli_inf[i].cthread);
	}
	CloseHandle(rthread);
	//�رռ����׽���
	closesocket(ser_sock);
	//����winsock2�Ļ���
	WSACleanup();
	cout << "������������������ֹͣ���񡪡�����" << endl;
	return 0;
}






