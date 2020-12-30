#include "WinSock2.h"
#include "zed.hpp"
#pragma comment(lib,"ws2_32.lib")


#define BLOCKSIZE 31584
//#define BLOCKSIZE 15792
//#define BLOCKSIZE 1974
//#define BLOCKSIZE 57600

#define BLOCKNUM 64
//#define BLOCKNUM 2

#define THREADNUM 4
#define THREADBLOCKNUM 32 
#include <thread>
SOCKET sockClient;

int nLen;
cv::Mat sourceMat;
cv::Mat resizeMat;
char* dataPoint;
char ip[20];
 
struct package//包格式
{
	char buf[BLOCKSIZE];//存放数据的变量
	int flag;//标志
};

void threadSend(int blockNum, int index,int port)
{
	SOCKADDR_IN addrRec;
	//int port = port;
	addrRec.sin_family = AF_INET;
	addrRec.sin_port = htons(port);
	addrRec.sin_addr.s_addr = inet_addr(ip);

	package sendPackage;
	char* threadPoint = dataPoint + (index*blockNum)*BLOCKSIZE;
	for (int i = 0, j = index * blockNum; i < blockNum; i++)
	{
		memcpy(sendPackage.buf, threadPoint, BLOCKSIZE);
		threadPoint += BLOCKSIZE;
		sendPackage.flag = i + j;
		int sendData = sendto(sockClient, (char*)(&sendPackage), sizeof(package), 0, (SOCKADDR*)&addrRec, nLen);
		if (sendData <= 0)
		{
			printf("发送data错误！错误代码：%d\t 第%d个分包错误\n", sendData, WSAGetLastError(), i);
		} 
		//else
		//	printf("发送第%d分包\n", sendPackage.flag);
	}
}

int main()
{


	printf("输入目标IP\n");
	scanf("%s", &ip);
	printf("目标IP%s\n", ip);
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "加载套接字失败" << std::endl;
		return 1;
	}


	SOCKADDR_IN addrRec;
	int port = 5099;
	addrRec.sin_family = AF_INET;
	addrRec.sin_port = htons(port);
	addrRec.sin_addr.s_addr = inet_addr(ip);


	sockClient = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockClient == SOCKET_ERROR)
	{
		std::cout << "创建套接字失败" << std::endl;
		return 1;
	}
	nLen = sizeof(SOCKADDR_IN);

	myZed zed;
	int width, height;

	//cv::Mat sendMat;

	int sendData;
	__int64 datasize;
	__int64 sendsize;

	zed.createCamera(&sourceMat, &width, &height);

	//dataPoint = (char*)sourceMat.datastart;
	package sendPackage;
	printf("开始发送。。。\n");
	int count = 0;
	double timer = (double)cv::getTickCount(), fps = 0;
	while (1)
	{
		if (!zed.cameraGrab())continue;
		else
		{
			//cv::resize(sourceMat, resizeMat, cv::Size(), 1, 1);
			//cv::resize(sourceMat, resizeMat, cv::Size(), 0.5, 0.5);
			//cv::resize(sourceMat, resizeMat, cv::Size(), 0.25, 0.25);

			dataPoint = (char*)sourceMat.datastart;
			//dataPoint = (char*)resizeMat.datastart;
			for (int i = 0; i < BLOCKNUM; i++)
			{
				memcpy(sendPackage.buf, dataPoint, BLOCKSIZE);
				dataPoint += BLOCKSIZE;
				sendPackage.flag = i;
				sendData = sendto(sockClient, (char*)(&sendPackage), sizeof(package), 0, (SOCKADDR*)&addrRec, nLen);
				if (sendData <= 0)
				{
					printf("发送data错误！错误代码：%d\t 第%d个分包错误\n", sendData, WSAGetLastError(), i);
				}

			}
			fps = ((double)cv::getTickCount() - timer) / cv::getTickFrequency();
			count++;
			if (fps > 1)
			{
				printf("每秒发送%d帧\n", count);
				count = 0;
				timer = (double)cv::getTickCount();
			}
		}
	}
	printf("发送结束。。。,共发送%d帧\n", count);
	system("pause");
}