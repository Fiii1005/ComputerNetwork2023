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
int res = 0; //接收文件的结果
int seraddr_len = sizeof(sockaddr_in);	//存储sockaddr缓冲区的大小
int cliaddr_len = sizeof(sockaddr_in);

//超时等待时间,默认250毫秒
int waittime = 250;


//设置超时时间
struct timeval tv;	//用于设置select函数的超时时间
fd_set readfds;		//文件描述符集合，用于监视文件描述符的状态

void setTO() {
	FD_ZERO(&readfds);	//初始化设置为空
	FD_SET(clisock, &readfds);	//设置套接字
	tv.tv_sec = 0;	//设置超时时间为1s
	tv.tv_usec = waittime;
}

//发送ACK包
void sendACK(int seq) {
	message* sendMes = new message();
	sendMes->FLAGS = ACK;
	sendMes->ackseq = seq;
	sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
	delete sendMes;
}

//发送标志位包
void sendFLAGS(int f) {
	message* sendMes = new message();
	sendMes->FLAGS = f;
	res=sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
	if (res > 0) {
		cout << "成功发送标志位：";
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
		cout << "发送数据包失败！错误代码：" << WSAGetLastError() << endl;
	}
	delete sendMes;
}


void connect() {
	int state = 0;
	bool flag = true;
	message* recvMes = new message();
	while (flag) {
		switch (state) {
		case 0: //向服务器端发送SYN位
			printTime();
			cout << "开始第一次握手！向服务器端发送SYN包！" << endl;
			sendFLAGS(SYN);
			state = 1;
			break;
		case 1: //在超时时间内收到SYN_ACK包，并发回ACK
			//判断是否超时
			setTO();
			select(clisock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(clisock, &readfds)) {
				//如果在超时时间内收到数据
				if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
					printTime();
					if (recvMes->FLAGS == SYN_ACK) {
						cout << "客户端发来SYN_ACK包！发回ACK数据包！" << endl;
						sendFLAGS(ACK);
						cout << "与服务器端建立连接！" << endl;
						cout << "————————————————————" << endl;
						flag = false; //完成连接
					}
					else {
						cout << "服务器端发送非SYN_ACK包！" << endl;
					}
				}
			}
			else { //如果发生超时
				printTime();
				cout << "超时未收到服务器端SYN_ACK！重发SYN包！" << endl;
				sendFLAGS(SYN);
			}
			break;
		}
	}
	delete recvMes;
}

//退出连接
void disconnect() {
	int state = 0;
	bool flag = true;

	message* recvMes = new message();

	while (flag) {
		switch (state) {
		case 0: //第一次挥手，发送FIN包
			printTime();
			cout << "结束连接，发送FIN包！" << endl;
			sendFLAGS(FIN);
			state = 1;
			break;
		case 1:
			//判断是否超时
			setTO();
			select(clisock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(clisock, &readfds)) {
				//如果在超时时间内收到数据
				printTime();
				if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
					if (recvMes->FLAGS == ACK) {
						cout << "服务器端发来ACK包！第二次挥手成功！" << endl;
						state = 2;
					}
					else {
						cout << "客户端发送非ACK包！" << endl;
					}
				}
			}
			else { //如果发生超时
				printTime();
				cout << "超时未收到客户端ACK！重发FIN包！" << endl;
				sendFLAGS(FIN);
			}
			break;
		case 2:
			res = recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len);
			if (res >= 0) {
				if (recvMes->FLAGS == FIN_ACK) {
					printTime();
					cout << "收到了来自服务器端的FIN_ACK包，第三次挥手成功！" << endl;
					state = 3;
				}
				else {
					printTime();
					cout << "服务器端发送非FIN_ACK包！" << endl;
				}
			}
			break;
		case 3: //向服务器端发送ACK包
			printTime();
			cout << "向服务器端发送ACK包，结束连接！" << endl;
			sendFLAGS(ACK);
			flag = false;
			break;
		}
	}
	delete recvMes;
}

int fileSize; //文件大小
string fileName; //文件名字
string filePath; //文件路径
char* fileBuffer; //文件缓冲区
int packetNum; //总文件数

//读取文件
bool readFile() {
	//cout << "请输入要传输的文件路径（绝对路径使用/）：" << endl;
	//cin.ignore();
	//getline(cin, filePath);
	cout << "请输入要传输的文件名（包括后缀）：" << endl;
	cin.ignore();
	getline(cin, fileName);
	//C:/Users/10141/Documents/Fi/2023Fall/7 ComputerNetwork/labs/3/3-1/send/
	filePath = "C:/Users/10141/Documents/Fi/2023Fall/7 ComputerNetwork/labs/3/实验3测试文件和路由器程序/测试文件/" + fileName;
	// fileName = "test.txt";
	ifstream f(filePath, ifstream::in | ios::binary);	//以二进制方式打开
	if (!f.is_open()) {
		cout << "文件无法打开！" << endl;
		return false;
	}
	f.seekg(0, std::ios_base::end);//以文件流指针定位到流的末尾
	fileSize = f.tellg();
	packetNum = fileSize % 1024 ? fileSize / 1024 + 1 : fileSize / 1024;
	cout << "文件大小为：" << fileSize << "比特！总共要发送" << packetNum << "个数据包" << endl;
	f.seekg(0, std::ios_base::beg); //将文件流指针重新定位到流的开始
	fileBuffer = new char[fileSize];
	if (fileBuffer == NULL) {
		cout << "内存分配失败！" << endl;
		f.close();
		return false;
	}
	f.read(fileBuffer, fileSize);
	f.close();
	return true;
}

//发送文件
void sendFile() {
	int allsendnum = 0; //所有发送的包数
	int lostnum = 0;//丢包数
	//先发送一个记录文件名的数据包，并设置HEAD标志位为1，表示开始文件传输
	message* sendMes = new message;
	message* recvMes = new message;
	printTime();
	clock_t start = clock();
	cout << "发送文件头数据包…………" << endl;
	char* fileNamec = new char[128];
	strcpy(fileNamec, fileName.c_str());
	sendMes->setHEAD(1, fileSize, fileNamec);
	sendMes->setchecksum((uint16_t*)sendMes);
	printSendMessage(sendMes);
	sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
	int sendnum = 0; //已发送文件包数量
	int state = 0;
	allsendnum++;
	bool flag = true;
	while (flag) {
		switch (state) {
		case 0:
			//判断是否超时
			setTO();
			select(clisock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(clisock, &readfds)) {
				//如果在超时时间内收到数据
				printTime();
				if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
					if (isACK(recvMes, 1)&&!isCorrupt(recvMes)) {
						cout << "收到ACK_1" << endl;
						if (sendnum == packetNum) {
							state = 4;
						}
						else {
							state = 1;
						}
					}
					else {
						cout << "接收的包已损坏或不是ACK_1包！" << endl;
					}
				}
				else {
					cout << "接收ACK包错误：" << WSAGetLastError() << endl;
				}
			}
			else { //如果发生超时
				printTime();
				cout << "超时未收到ACK！重发数据包！" << endl;
				allsendnum++;
				lostnum++;
				sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
			}
			break;
		case 1:
			//发送下一数据包
			printTime();
			cout << "正在发送第" << sendnum << "个数据包！" << endl;
			sendMes->clearMes();
			//如果是最后一个包
			if (sendnum == packetNum-1) {
				sendMes->FLAGS=TAIL;
			}
			sendMes->messeq = 0;
			sendMes->fillData(0, (fileSize - sendnum * 1024) > 1024 ? 1024 : fileSize - sendnum * 1024, fileBuffer + sendnum * 1024);
			sendMes->setchecksum((uint16_t*)sendMes);
			cout << "test校验和为：" << sendMes->checksum << endl << endl;
			res = sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
			if (res > 0) {
				allsendnum++;
				cout << "数据包发送成功！" << endl;
				printSendMessage(sendMes);
				sendnum++;
				state = 2;
			}
			else {
				cout << "数据包发送失败！" << endl;
			}
			break;
		case 2:
			//判断是否超时
			setTO();
			select(clisock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(clisock, &readfds)) {
				//如果在超时时间内收到数据
				printTime();
				if (recvfrom(clisock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(seraddr), &seraddr_len) > 0) {
					if (isACK(recvMes, 0)&&!isCorrupt(recvMes)) {
						cout << "收到ACK_0" << endl;
						if (sendnum == packetNum) {
							state = 4;
						}
						else {
							state = 3;
						}
					}
					else {
						cout << "接收的包已损坏或不是ACK_0包！" << endl;
					}
				}
				else {
					cout << "接收ACK包错误：" << WSAGetLastError() << endl;
				}
			}
			else { //如果发生超时
				printTime();
				lostnum++;
				allsendnum++;
				cout << "超时未收到ACK！重发数据包！" << endl;
				sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
			}
			break;
		case 3:
			//发送下一数据包
			printTime();
			cout << "正在发送第" << sendnum << "个数据包！" << endl;
			sendMes->clearMes();
			//如果是最后一个包
			if (sendnum == packetNum - 1) {
				sendMes->FLAGS = TAIL;
			}
			sendMes->fillData(1, (fileSize - sendnum * 1024) > 1024 ? 1024 : fileSize - sendnum * 1024, fileBuffer + sendnum * 1024);
			sendMes->setchecksum((uint16_t*)sendMes);
			cout << "test校验和为：" << sendMes->checksum << endl << endl;
			res = sendto(clisock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&seraddr, seraddr_len);
			if (res > 0) {
				cout << "数据包发送成功！" << endl;
				allsendnum++;
				printSendMessage(sendMes);
				sendnum++;
				state = 0;
			}
			else {
				cout << "数据包发送失败！" << endl;
			}
			break;
		case 4: //当前文件已经发完了
			flag = false;
			printTime();
			cout << "当前文件发送完毕。" << endl;
			clock_t end = clock();
			cout << "总传输时间为：" << (end - start)/CLOCKS_PER_SEC << "秒" << endl;
			cout << "吞吐率为：" << (float)fileSize / ((end - start)*1000 / CLOCKS_PER_SEC) << "比特/毫秒" << endl;
			cout << "错误（丢包+超时）率为：" << (float)lostnum / allsendnum << endl;
			break;
		}
	}
	delete sendMes;
	delete recvMes;
	delete []fileNamec;
}

int main() {
	//初始化socket环境
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "socket初始化失败" << endl;
		return 0;
	}
	//创建socket
	clisock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//ipv4地址，使用UDP数据报套接字，不指定协议
	if (clisock == INVALID_SOCKET) {
		cout << "socket创建失败" << endl;
		return -1;
	}

	//初始化地址
	//默认IP
	//定义套接字的IP和端口
	memset(&seraddr, 0, sizeof(sockaddr_in)); //清空addr内存部分的值
	seraddr.sin_family = AF_INET;
	//使用htons将u_short将主句转换为IP网络字节顺序（大端）
	seraddr.sin_port = htons(8000);
	//使用inet_addr函数将IPv4地址的字符串转换为s_addr结构
	seraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//路由器IP：127.0.0.1 端口：8001
	//服务器IP：127.0.0.1 端口：8000


	cout << "请设置最大超时等待时间（毫秒）：" << endl;
	cin >> waittime;

	cout << "客户端初始化完毕" << endl;

	connect();
	while (true) {
		cout << "是否要发送文件？是：0/退出：1" << endl;
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
			cout << "指令错误！" << endl;
		}
	}
	//断开连接
	disconnect();
	
	res = closesocket(clisock);
	if (res == SOCKET_ERROR) {
		cout<< "关闭套接字错误：" << WSAGetLastError() << endl;
		return 1;
	}

	printTime();
	cout << "程序退出……" << endl;
	WSACleanup();

	delete fileBuffer;

	
	return 0;
}