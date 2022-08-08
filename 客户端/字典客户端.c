#include<stdio.h>
#include<string.h>
#include<strings.h>
#include<stdlib.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include"字典客户端.h"

//准备函数
int open_sock()
{
    char buf[20] = "\0";
    printf("端口8888，请输入服务器的IP>>>>");
    fgets(buf,sizeof(buf),stdin);
	//创建套接字
	int sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd < 0)
	{
		printf("创建失败\n");
		return -1;
	}
	//填充结构体信息
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port   = htons(8888);
	sin.sin_addr.s_addr = inet_addr(buf);
	//sin.sin_addr.s_addr = inet_addr("192.168.5.11");
	//允许端口快速重用
	int reuae = 1;
	if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuae,sizeof(reuae)) < 0)
	{
		perror("setsockopt");
		return 0;
	}

	//等待服务器打开                                            
	do
	{
		if(connect(sfd,(struct sockaddr*)&sin,sizeof(sin)) < 0)
		{
			if(errno != 111)
			{
				perror("connect");
				return 0;
			}
			printf("等待服务器打开\n");
			sleep(1);
		}
		else{errno = 0;}
	}while((errno == 111&& 3==sfd));
	return sfd;
}
//一级目录
void one_dir()
{
	system("clear");
	putchar(10);
	printf("****************************\n");
	printf("**********1、注册***********\n");
	printf("**********2、登录***********\n");
	printf("**********3、退出***********\n");
	printf("****************************\n");
	printf("请输入>>>>>>");
	fflush(stdout);
	return;
}
//二级目录
void two_dir()
{
	system("clear");
	putchar(10);
	printf("****************************\n"); 
	printf("*******1、查询**************\n");
	printf("*******2、退出系统**********\n");
	printf("*******3、返回上一级********\n");
	printf("*******4、历史记录查询******\n");
	printf("****************************\n");
	printf("请输入>>>>>>");
	fflush(stdout);
	return;
}

//发送数据
void send_out(int sfd)
{ 	
	char addr[128]= "\0";
	fgets(addr,sizeof(addr),stdin);
	addr[strlen(addr)-1] = 0;
	send(sfd,addr,strlen(addr),0);

	return;
}

//接收数据
int recv_input(int sfd)
{
	char buf[128] = "\0";
	ssize_t res = 0;
	res = recv(sfd,buf,sizeof(buf),0);
	if(res == 0)
	{
		return -1;
	}

	//判断接收到的数据包属于指令包还是数据包
	if((strlen(buf)<7))
	{
		//指令包调用信息提示函数
		input_all(buf);	

	}
	else{
		//数据包直接在终端打印数据
		printf("%s\n",buf);}
	return 0;
}

//功能提示函数
void input_all(char buf[])
{
	/***********************************************
	  提示:下面 “ptintf” 没有加\n刷新缓冲区是防止
	  再调用目录函数时，提示信息被覆盖
	 ************************************************/

	int j=0; 		//用于判断是否有粘包现象，粘了几层
	int i=0; 		//用于处理有粘包的数据包分离 

	//处理有无粘包的提示页面
	switch((strlen(buf)/2))
	{
		case 1 :
			j = 2;
			break;
		case 2:
			j = 4;
			break;
		case 3:
			j = 6;
			break;
		default:
			printf("出错了\n");
			break;
	}
	for(i=0;i<j;i=i+2)
	{
		switch(buf[i])
			{
			case '0':
					switch(buf[i+1])
					{
					case '@':
						printf("<<<密码输入错误3次将自动返回主页>>>\n");
						break;
					case '#':
						printf("输入任意字符清屏>>> ：");
						fflush(stdout);
						while(getchar()!=10);
						system("clear");
						break;
					case '*':
						printf("是否退出回到主页【y/n】 ：");
						fflush(stdout);
						break;
					case '1':
						printf("输入用户名>>>>");
						fflush(stdout);
						break;
					case '2':
						printf("输入密码>>>>");
						fflush(stdout);
						break;
					case '3':
						printf("当前库中没有用户\n");
						break;
					case '4':
						printf("该用户未注册");
						break;
					case '5':
						printf("密码/用户错误\n");
						break;
					case '6':
						printf("重复登录\n");
						printf("是否回到主页【y/n】 ：");
						fflush(stdout);
						break;
					case '7':
						printf("登录成功");
						break;
					case '8':
						printf("该用户名已存在\n");
						printf("是否回到主页【y/n】");
						fflush(stdout);
						break;
					case '9':
						printf("服务器已连接、字典数据库已准备完毕");
						break;
					}
				break;
			case '1':
					switch(buf[i+1])
					{
					case '0':
						printf("输入格式错误、请重新输入>>>>");
						fflush(stdout);
						break;
					case '1':
						printf("请输入要查找的单词>>>>");
						fflush(stdout);
						break;
					case '2':
						printf("未找到单词\n");
						break;
					case '3':
						printf("注册成功,并已登录");
						break;
					case '4':
						printf("已退出单词查询");
					case '5':
						//调用一级目录
						one_dir();
						break;
					case '6':
						//调用二级目录
						two_dir();
						break;
					case '7':
						printf("密码错误次数已达上限");
						break;
					case '8':
						printf("当前用户暂未历史记录\n");
						break;
					case'9':
						printf("历史记录查询完成\n");
						break;
					}
				break;
			}
		}
	return;
}
