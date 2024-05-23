#include <iostream>
#include <winsock2.h>
#include <string>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

//WSA������ʼ��
//�����ͻ���
//���ӷ�������
//�������߳̽��շ���������Ϣ
//���߳������˷�����Ϣ
//���ͽ�������
//�ر��߳�
//�رշ�������
//����WSA����

SOCKET cli_sock;					//�ͻ����׽���
sockaddr_in cli_addr;				//�ͻ���IP�Ͷ˿�
int saddr_len=sizeof(sockaddr_in);	//sockaddr_in�ṹ����
HANDLE* recv_handle;				//�����߳�

//������Ϣ�߳�
DWORD WINAPI ReceiveThread(LPVOID cs) {
	//ѭ��������Ϣ
	while (true) {
		char recv_buf[100] = { 0 };
		int ret = 100;
		ret= recv(cli_sock, recv_buf, 100, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() != 10053) {
				cout << "�������������˴���" << WSAGetLastError() << "������" << endl;
			}
			return 0;
		}
		if (!strcmp(recv_buf,"exit")) {
			cout << "������������������ֹͣ���񡪡�����" << endl;
			return 0;
		}
		if (ret == 0) {
			cout << "������������������ֹͣ���񡪡�����" << endl;
			return 0;
		}
		cout << recv_buf << endl;
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

	//�����ͻ���Socket
	cout << "���������ͻ��ˡ�������" << endl;
	cli_sock = socket(
		AF_INET,		//ʹ��IPv4��ַ
		SOCK_STREAM,	//ʹ����ʽ�׽���
		IPPROTO_TCP		//ʹ��TCPЭ��
	);

	//����IP�Ͷ˿�
	cli_addr.sin_family = AF_INET;		//IPv4
	cli_addr.sin_port = htons(8000);	//�˿�
	cli_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//���ӷ�������
	if (connect(cli_sock, (SOCKADDR*)&cli_addr, saddr_len) == SOCKET_ERROR) {
		cout << "���������ӷ�������ʧ�ܣ�" << WSAGetLastError() << endl;
		return 0;
	}

	//�������߳̽��շ������Ϣ
	HANDLE rt = CreateThread(NULL, 0, ReceiveThread, (LPVOID)&cli_sock, 0, NULL);
	
	//���߳�ѭ��������Ϣ
	while (true) {
		char send_buf[100] = { 0 };
		cin.getline(send_buf, 100);
		int ret = 100;
		ret = send(cli_sock, send_buf, 100, 0);
		if (!strcmp(send_buf, "exit")) {
			cout << "���������ͻ��˹رա�������" << endl;
			break;
		}
		if (ret == -1) {
			if (WSAGetLastError() == 10054) {
				cout << "������������������ֹͣ���񡪡�����" << endl;
			}
		}
	}
	//�ر������߳�
	CloseHandle(rt);
	//�ر��׽���
	closesocket(cli_sock);
	//����WSA����
	WSACleanup();
	return 0;
}


