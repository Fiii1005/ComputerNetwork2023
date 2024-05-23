#include <iostream>
#include <winsock2.h>
#include <string>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

//WSA环境初始化
//创建客户端
//连接服务器端
//创建新线程接收服务器端信息
//主线程向他人发送消息
//发送结束命令
//关闭线程
//关闭服务器端
//清理WSA环境

SOCKET cli_sock;					//客户端套接字
sockaddr_in cli_addr;				//客户端IP和端口
int saddr_len=sizeof(sockaddr_in);	//sockaddr_in结构长度
HANDLE* recv_handle;				//接收线程

//接收消息线程
DWORD WINAPI ReceiveThread(LPVOID cs) {
	//循环接收消息
	while (true) {
		char recv_buf[100] = { 0 };
		int ret = 100;
		ret= recv(cli_sock, recv_buf, 100, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() != 10053) {
				cout << "！！！服务器端错误：" << WSAGetLastError() << "！！！" << endl;
			}
			return 0;
		}
		if (!strcmp(recv_buf,"exit")) {
			cout << "————服务器端已停止服务————" << endl;
			return 0;
		}
		if (ret == 0) {
			cout << "————服务器端已停止服务————" << endl;
			return 0;
		}
		cout << recv_buf << endl;
	}
	return 0;
}


int main() {
	//初始化Winsock
	//创建wsadata的对象
	WSADATA wsaData;
	//返回初始化结果
	int wsaRe;
	wsaRe = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaRe != 0) {
		cout << "！！！WSA环境初始化失败！！！" << endl;
		return 0;
	}

	//创建客户端Socket
	cout << "————客户端————" << endl;
	cli_sock = socket(
		AF_INET,		//使用IPv4地址
		SOCK_STREAM,	//使用流式套接字
		IPPROTO_TCP		//使用TCP协议
	);

	//设置IP和端口
	cli_addr.sin_family = AF_INET;		//IPv4
	cli_addr.sin_port = htons(8000);	//端口
	cli_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//连接服务器端
	if (connect(cli_sock, (SOCKADDR*)&cli_addr, saddr_len) == SOCKET_ERROR) {
		cout << "！！！连接服务器端失败：" << WSAGetLastError() << endl;
		return 0;
	}

	//创建新线程接收服务端信息
	HANDLE rt = CreateThread(NULL, 0, ReceiveThread, (LPVOID)&cli_sock, 0, NULL);
	
	//主线程循环发送消息
	while (true) {
		char send_buf[100] = { 0 };
		cin.getline(send_buf, 100);
		int ret = 100;
		ret = send(cli_sock, send_buf, 100, 0);
		if (!strcmp(send_buf, "exit")) {
			cout << "————客户端关闭————" << endl;
			break;
		}
		if (ret == -1) {
			if (WSAGetLastError() == 10054) {
				cout << "————服务器端已停止服务————" << endl;
			}
		}
	}
	//关闭所有线程
	CloseHandle(rt);
	//关闭套接字
	closesocket(cli_sock);
	//清理WSA环境
	WSACleanup();
	return 0;
}


