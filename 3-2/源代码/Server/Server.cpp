#include <fstream>
#include <Winsock2.h>
#include "message.h"
#include <Windows.h>
#include <random>
#include <iostream>

using namespace std;

//链接dll的lib
#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE sizeof(message)
//每次延时时间=随机数*延时基数(10毫秒)
int DELAY_BASE = 10;
//丢包率 设为<0时不会产生丢包
float LOSS_RATE = -1;
//延时率 设为<0时不会产生丢包
float DELAY_RATE = -1;


SOCKET sersock;		//发送端套接字
sockaddr_in seraddr;	//套接字IP和端口号
sockaddr_in cliaddr;	//客户端的IP和端口号
int cliaddr_len = sizeof(sockaddr_in);	//存储cliaddr缓冲区的大小

//设置超时时间
struct timeval tv;	//用于设置select函数的超时时间
fd_set readfds;		//文件描述符集合，用于监视文件描述符的状态


default_random_engine randomGen;
uniform_real_distribution<float> randomNum(0.0, 1.0);  // 通过随机数设置丢包

void setTO() {
	FD_ZERO(&readfds);	//初始化设置为空
	FD_SET(sersock, &readfds);	//设置套接字
	tv.tv_sec = 1;	//设置超时时间为1s
}

//发送ACK包
void sendACK(int seq) {
	message* sendMes = new message;
	sendMes->FLAGS = ACK;
	sendMes->ackseq = seq;
	sendMes->setchecksum((uint16_t*)sendMes);
	int err = 0;
	err = sendto(sersock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&cliaddr, cliaddr_len);
	if (err > 0) {
		cout << "发送ACK_" << seq << "成功！" << endl;
	}
	else {
		cout << "发送ACK_" << seq << "失败！" << endl;
	}
	delete sendMes;
}

//发送标志位包
void sendFLAGS(int f) {
	message* sendMes = new message();
	sendMes->FLAGS = f;
	int err = 0;
	err = sendto(sersock, (char*)sendMes, BUFFER_SIZE, 0, (SOCKADDR*)&cliaddr, cliaddr_len);
	printTime();
	if (err > 0) {
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

//三次握手建立连接
void connect() {
	int state = 0; //标识目前握手的状态
	bool flag = true;  //表明是否完成握手
	int res = 0;
	message* recvMes = new message;
	/*
	int WSAAPI recvfrom(
	  [in]                SOCKET   s,		//绑定套接字
	  [out]               char     *buf,	//传入数据的缓冲区
	  [in]                int      len,		//缓冲区长度
	  [in]                int      flags,	//
	  [out]               sockaddr *from,	//sockaddr结构，该缓冲区将在返回时保留源地址
	  [in, out, optional] int      *fromlen	//存储sockaddr缓冲区的大小
	);
	*/
	u_long mode = 1;
	int err = 0;
	while (flag) {
		switch (state) {
		case 0: //等待客户端发送SYN数据包状态
			// 设置阻塞模式
			setTO();
			err=select(sersock + 1, &readfds, NULL, NULL, NULL); //timeout设置为NULL表示等待无限长时间
			if (FD_ISSET(sersock, &readfds)) { //如果收到数据
				printTime();
				res = recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len);
				if (res > 0) {	//接收到数据
					if (recvMes->FLAGS == SYN) {
						cout << "收到来自客户端的连接请求！第一次握手成功！" << endl;
						//服务器端发送SYN_ACK包，第二次握手
						sendFLAGS(SYN_ACK);
						/*
						int WSAAPI sendto(
						  [in] SOCKET         s,		//连接的套接字
						  [in] const char     *buf,		//要传输的数据的缓冲区指针
						  [in] int            len,		//buf指向的数据的长度
						  [in] int            flags,	//调用方式的标志
						  [in] const sockaddr *to,		//目标地址
						  [in] int            tolen		//地址大小
						);
						*/
						state = 1; //转状态1
					}
					else {
						cout << "第一次握手数据包不匹配！" << endl;
					}
				}
				else {
					cout << "接收数据不正确：" << WSAGetLastError() << endl;
				}
			}
			else {
				printTime();
				cout << "接收数据报错误：" << WSAGetLastError() << endl;
			}
			break;
		case 1:	//接收客户端的ACK=1数据包
			//select函数确定一个或多个套接字的状态，并在必要时等待执行同步I/O。
			//判断是否超时
			setTO();
			select(sersock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(sersock, &readfds)) {
				//如果在超时时间内收到数据
				if (recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len) > 0) {
					printTime();
					if (recvMes->FLAGS == ACK) {
						cout << "客户端发来ACK包！第三次握手成功！" << endl;
						cout << "————————————————————————————————" << endl;
						flag = false;
					}
					else {
						cout << "客户端发送非ACK包！" << endl;
					}
				}
			}
			else { //如果发生超时
				printTime();
				cout << "超时未收到客户端ACK！重发SYN_ACK包！" << endl;
				sendFLAGS(SYN_ACK);
			}
			break;
		}
	}
	delete recvMes;
}

//断开连接
void disconnect() {
	int state = 0; //标识目前挥手状态
	bool flag = true; //为true时没有挥手结束
	int res = 0;
	message* recvMes = new message();

	while (flag) {
		switch (state) {
		case 0:
			printTime();
			cout << "客户端发起结束连接请求！" << endl;
			//向客户端发送ACK包
			sendFLAGS(ACK);
			state = 1;
			break;
		case 1:
			printTime();
			cout << "开始第三次挥手！发送FINACK包！" << endl;
			sendFLAGS(FIN_ACK);
			state = 2;
			break;
		case 2:
			//判断是否超时
			setTO();
			select(sersock + 1, &readfds, NULL, NULL, &tv);
			if (FD_ISSET(sersock, &readfds)) {
				//如果在超时时间内收到数据
				printTime();
				if (recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len) > 0) {
					if (recvMes->FLAGS == ACK) {
						cout << "客户端发来ACK包！第四次挥手成功！" << endl;
						cout << "————————————————————————————————" << endl;
						flag = false;
					}
					else {
						cout << "客户端发送非ACK包！" << endl;
					}
				}
			}
			else { //如果发生超时
				printTime();
				cout << "超时未收到客户端ACK！重发FIN_ACK包！" << endl;
				sendFLAGS(FIN_ACK);
			}
			break;
		}
	}
	delete recvMes;
}

int fileSize = 0; //总文件长度
char* fileName;
char* fileBuffer;
int packetnum;	//某个文件需要的数据包数量
unsigned int recvSize; //累计收到的文件位置（用于写入缓冲区）

//保存文件
void saveFile() {
	string filePath = "C:/Users/10141/Documents/Fi/2023Fall/7 ComputerNetwork/labs/3/3-1/save/";
	for (int i = 0; fileName[i]; i++) {
		filePath += fileName[i];
	}
	cout << "文件路径为：" << filePath << endl;
	/*ofstream fout;
	fout.open(filePath, ios::app);
	for (int i = 0; i < fileSize; i++) {
		fout << fileBuffer[i];
	}*/
	ofstream fout(filePath, ios::binary | ios::out);
	fout.write(fileBuffer, fileSize); // 这里还是size,如果使用string.data或c_str的话图片不显示，经典深拷贝问题
	printTime();
	cout << "当前文件保存成功！" << endl;
	fout.close();
}


//接收文件
void receiveFile() {
	bool flag = true;
	int state = 0;
	int res = 0;	//接收返回
	int expectedseqnum = 0; //期待接收的下一个包的序号
	

	message* recvMes = new message();


	while (flag) {	//循环接收文件数据包
		switch (state) {
		case 0:	//等待数据包头文件状态
			res = recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len);
			if (res > 0) {
				if (randomNum(randomGen) <= LOSS_RATE) {//主动丢包
					printTime();
					cout << "主动丢包" << endl;
				}
				else {
					if (randomNum(randomGen) <= DELAY_RATE) {//延时
						int time = DELAY_BASE * randomNum(randomGen);
						Sleep(time);
						printTime();
						cout << "延时" << time << "毫秒" << endl;
					}
					printTime();
					printRecvMessage(recvMes);
					if (recvMes->FLAGS == FIN) {
						flag = false;
						disconnect();
					}
					else if (isCorrupt(recvMes) || recvMes->messeq != 0) {
						cout << "当前文件已接收完毕，等待发送下一文件" << endl;
						sendACK(expectedseqnum - 1);
					}
					else if (!isCorrupt(recvMes) && recvMes->messeq == 0) {
						if (recvMes->FLAGS == HEAD) {
							expectedseqnum = 1;
							cout << "接收数据包头成功！" << endl;
							//初始化接收文件缓冲区
							recvSize = 0;
							fileSize = recvMes->filelen;
							fileBuffer = new char[fileSize];
							fileName = new char[128];
							memcpy(fileName, recvMes->data, strlen(recvMes->data) + 1);
							//计算文件包数量
							packetnum = fileSize % 1024 ? fileSize / 1024 + 1 : fileSize / 1024;
							cout << "开始接收来自发送端的文件，文件名为：" << fileName << endl;
							cout << "文件大小为：" << fileSize << "比特，总共需要接收" << packetnum << "个数据包" << endl;
							cout << "等待发送文件数据包…………" << endl;
							//发送ACK_0数据包
							sendACK(0);
							state = 1;
						}
						else {
							cout << "收到的数据包不是文件头，等待发送端重传……" << endl;
						}
					}
				}
			}
			break;
		case 1:
			res = recvfrom(sersock, (char*)recvMes, BUFFER_SIZE, 0, (SOCKADDR*)&(cliaddr), &cliaddr_len);
			if (res > 0) {
				if (randomNum(randomGen) <= LOSS_RATE) {//主动丢包
					printTime();
					cout << "主动丢包" << endl;
				}
				else {
					if (randomNum(randomGen) <= DELAY_RATE) {//延时
						int time = DELAY_BASE * randomNum(randomGen);
						Sleep(time);
						printTime();
						cout << "延时" << time << "毫秒" << endl;
					}
					printTime();
					printRecvMessage(recvMes);
					//累积确认
					if (!isCorrupt(recvMes) && recvMes->messeq == expectedseqnum) {
						//收到了目标的包
						memcpy(fileBuffer + recvSize, recvMes->data, recvMes->filelen);
						recvSize += recvMes->filelen;
						sendACK(expectedseqnum);
						expectedseqnum++;
						if (recvMes->FLAGS == TAIL) {
							//如果是最后一个包，进入状态0，也就是等待头文件或者断开连接
							state = 0;
							cout << "当前文件接收完毕！" << endl;
							cout << "——————————————————" << endl;
							//保存文件
							saveFile();
						}
						else {
							cout << endl << "文件包接收成功！" << endl;
						}
					}
					else {
						cout << endl << "包损坏或序号错误" << endl;
						sendACK(expectedseqnum - 1);
					}
				}
			}
			else {
				cout << "接受包错误！" << endl;
			}
			break;
		}
	}
	delete recvMes;
}

int main() {
	//加载socket库
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "socket初始化失败" << endl;
		return 0;
	}
	//创建socket
	sersock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//ipv4地址，使用UDP数据报套接字，不指定协议
	if (sersock == INVALID_SOCKET) {
		cout << "socket创建失败" << endl;
		return -1;
	}

	cout << "服务器端初始化完毕" << endl;
	cout << "IP地址：127.0.0.1 端口号：8000" << endl;

	//初始化地址
	//默认IP
	//定义套接字的IP和端口
	memset(&seraddr, 0, sizeof(sockaddr_in)); //清空addr内存部分的值
	seraddr.sin_family = AF_INET;
	//使用htons将u_short将主句转换为IP网络字节顺序（大端）
	string serIP = "127.0.0.1";
	int port = 8000;
	/*cout << "请输入IP地址：" << endl;
	cin >> serIP;
	cout << "请输入端口号：" << endl;
	cin >> port;*/
	char* portc = new char[20];
	memcpy(portc, serIP.c_str(),20);

	seraddr.sin_port = htons(port);
	//使用inet_addr函数将IPv4地址的字符串转换为s_addr结构
	seraddr.sin_addr.s_addr = inet_addr(portc);
	//将套接字绑定到本地IP和端口
	if (bind(sersock, (SOCKADDR*)&seraddr, sizeof(SOCKADDR))==-1) {
		//如果绑定发生错误
		cout << "套接字绑定发生错误：" << WSAGetLastError() << endl;
		return 0;
	}

	cout << "请设置丢包率（0~1）（设为<0时不会发生丢包）：" << endl;
	cin >> LOSS_RATE;
	cout << "请设置延时率（0~1）（设为<0时不会发生延时）：" << endl;
	cin >> DELAY_RATE;
	/*cout << "请设置超时时间（毫秒）：" << endl;
	cin >> DELAY_BASE;*/

	cout << "服务器端启动成功，等待客户端建立连接" << endl;

	connect();

	receiveFile();

	//关闭socket
	int res = closesocket(sersock);
	if (res == SOCKET_ERROR) {
		cout << "关闭套接字错误：" << WSAGetLastError() << endl;
		return 1;
	}
	printTime();
	cout << "程序退出……" << endl;
	WSACleanup();

	delete fileBuffer;
	delete fileName;
	delete []portc;

	return 0;
}