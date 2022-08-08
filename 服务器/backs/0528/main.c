#include<stdio.h>
//#include<strings.h>
#include<netinet/in.h>
//#include<arpa/inet.h>
//#include<sys/types.h>
#include<sys/socket.h>
#include<sqlite3.h>
#include<pthread.h>
#include"字典.h"

int main()
{

	//调用准备函数
	int sfd = sfd_fd();
	
	//创建负责通讯的结构体
	struct sockaddr_in cin;
	struct msg prd;
	int len = sizeof(cin);
	int fsd = 0;
	pthread_t tid;


	while(1)
	{

		//主线程负责链接
		fsd = accept(sfd,(struct sockaddr*)&cin,&len);
		if(fsd <0)
		{
			ERR_R("accept");
			return -1;
		}
		prd.newfd = fsd;
		prd.cin = cin;
		//创建线程
		pthread_create(&tid,NULL,each_other,&prd);

	}
	return 0;
}



