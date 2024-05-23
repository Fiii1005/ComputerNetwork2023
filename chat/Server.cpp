#include <iostream>
#include <Winsock2.h>
#include <string>
using namespace std;

//链接dll的lib
#pragma comment(lib, "ws2_32.lib")

//WSA环境初始化
//创建服务端
//开启监听
//-》新线程：客户端加入
//-》将客户端存在全局结构数组中
//---》每个客户端一个线程
//---》每收到一条消息就进行广播
//主线程判断是否退出服务
//关闭线程
//关闭服务器端
//清理WSA环境

SOCKET ser_sock;					//服务器端套接字
sockaddr_in ser_addr;				//套接字IP和端口号
int saddr_len = sizeof(sockaddr_in);//sockaddr_in的长度
bool bExit = false;					//判断是否退出

//存储客户端信息的结构
int cnum = 0;		//客户端个数
struct client {
	int cid=-1;			//客户端编号
	SOCKET cli_sock;	//客户端套接字
	HANDLE* cthread;	//客户端线程
}cli_inf[20];
//广播客户端函数
void bc(int cid, char* send_buf) {
	for (int i = 0; i < cnum; i++) {
		if (i == cid) {
			continue;
		}
		send(cli_inf[i].cli_sock, send_buf, 100, 0);
	}
	return;
}

//接收客户端信息线程函数
DWORD WINAPI ClientThread(LPVOID cc) {
	//与客户端通讯，接收并发送信息
	int cid = (int)cc;
	cout << "——客户端" << cid << "加入聊天室——" << endl;
	//发送数据
	char send_buf[100] = { 0 };
	sprintf_s(send_buf,100, "客户端 %d 成功加入聊天室！\n", cid);
	bc(-1, send_buf);	//广播
	//循环接收客户端数据
	while (true) {
		char recv_buf[100] = { 0 };
		int ret = 100;	//表示recv()函数返回值
		ret = recv(cli_inf[cid].cli_sock, recv_buf, 100, 0);
		if (ret == SOCKET_ERROR) {
			if (WSAGetLastError() == 10053) {
				break;
			}
			cout << "！！！客户端" << cid << "接收错误："<<WSAGetLastError()<<"！！！" << endl;
			break;
		}
		if (!strcmp(recv_buf,"exit")) {
			cout << "### 客户端" << cid << "已退出聊天室 ###" << endl;
			memset(send_buf, '0', sizeof(send_buf));	//清空发送缓冲区
			sprintf_s(send_buf,100, "### 客户端 %d 已退出聊天室！### \n", cid);
			bc(cid, send_buf);
			break;
		}
		memset(send_buf, '0', sizeof(send_buf));
		sprintf_s(send_buf,100, "客户端 %d : %s \n", cid, recv_buf);
		cout << send_buf;
		bc(cid, send_buf);
	}
	return 0;
}

//接收客户端线程
DWORD WINAPI ReceiveThread(LPVOID num) {
	//线程循环接收客户端的连接
	while (true) {
		client* c = &cli_inf[cnum];
		sockaddr_in addrClient;
		//返回与client通讯的socket
		c->cli_sock = accept(ser_sock, (SOCKADDR*)&addrClient, &saddr_len);
		c->cid = cnum;
		if (c->cli_sock != INVALID_SOCKET) {
			//如果返回的不是无效套接字
			//创建线程，接收该客户端的信息并转发
			HANDLE cThread = CreateThread(NULL, 0, ClientThread, (LPVOID)c->cid, 0, NULL);
			c->cthread = &cThread;
		}
		cnum++;
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

	cout << "————服务器端————" << endl;
	//创建套接字
	ser_sock = socket(
		AF_INET,		//使用IPv4地址
		SOCK_STREAM,	//使用流式套接字
		IPPROTO_TCP		//使用TCP协议
	);
	//定义套接字的IP和端口
	memset(&ser_addr, 0, sizeof(sockaddr_in)); //清空addr内存部分的值
	ser_addr.sin_family = AF_INET;
	//使用htons将u_short将主句转换为TCP/IP网络字节顺序（大端）
	ser_addr.sin_port = htons(8000);
	//使用inet_addr函数将IPv4地址的字符串转换为s_addr结构
	ser_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//将套接字绑定到本地IP和端口
	if (bind(ser_sock, (SOCKADDR*)&ser_addr, saddr_len)) {
		//如果绑定发生错误
		cout << "套接字绑定发生错误："<< WSAGetLastError() << endl;
		return 0;
	}

	//开启监听
	if (listen(ser_sock, 20) != 0) {
		cout << "监听错误" << endl;
		return 0;
	}
	cout << "…………服务器端正在监听…………" << endl;

	//接收客户端的连接的线程
	HANDLE rthread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)&cnum, 0, NULL);

	//死循环判断主线程是否退出
	while (true) {
		string command = "0";
		getline(cin,command);
		if (command == "exit") {
			//广播各客户端表示服务器端已停止服务
			char main_send[100] = { 0 };
			sprintf_s(main_send, 100, "exit");
			bc(-1, main_send);
			break;
		}
	}
	//退出服务器端指令
	//关闭所有创建的线程
	for (int i = 0; i < cnum; i++) {
		CloseHandle(*cli_inf[i].cthread);
	}
	CloseHandle(rthread);
	//关闭监听套接字
	closesocket(ser_sock);
	//清理winsock2的环境
	WSACleanup();
	cout << "————服务器端已停止服务————" << endl;
	return 0;
}






