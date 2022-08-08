#include<stdio.h>
#include<string.h>
#include<strings.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<pthread.h>
#include<sqlite3.h>
#include<unistd.h>
#include"字典.h"



char name[128] ="";

//准备工作
int sfd_fd()
{
#if 0
	char buf[20] = "\0";
	printf("请输入你要绑定服务器的IP>>>>");
	fgets(buf,sizeof(buf),stdin);
#endif
	//创建套接字
	int sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd < 0)
	{
		ERR_R("socket");
		return -1;
	}

	//允许端口快速重用
	int reuse = 1;
	if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse)) < 0)
	{
		ERR_R("setsockopt");
		return -1;
	}
	//填充结构体
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port   = htons(PORT);
	sin.sin_addr.s_addr = inet_addr(IP);
	//sin.sin_addr.s_addr = inet_addr(buf);
	//绑定IP和端口
	if(bind(sfd,(struct sockaddr*)&sin,sizeof(sin))<0)
	{
		ERR_R("bind");
		return -1;
	}
	//设为被动监听
	if(listen(sfd,10) < 0)
	{
		ERR_R("listen");
		return -1;
	}
	return sfd;
}


//分线程
void* each_other(void* arg)
{
	ssize_t res;
	char buf[128] = "";
	struct msg prd = *(struct msg*)arg;
	int fsd = prd.newfd;
	//分离线程
	pthread_detach(pthread_self());

	printf("【%s:%d】客户端已连接\n",inet_ntoa(prd.cin.sin_addr),ntohs(prd.cin.sin_port));

	//创建数据库并打开
	sqlite3 *db = sqlite_db(fsd);

	//给客户端发送指令
	send(fsd,"15",2,0); 		//一级目录（注册、登录、退出）
	recv(fsd,buf,sizeof(buf),0);
	switch(buf[0])
	{
		case '1' :
			//注册函数
			user_name(db,fsd);
			break;
		case '2' :
			//登录函数
			long_in_name(db,fsd);
			break;
		case '3' :
			//退出
			goto END1;
			break;
	}

	//给客户端发送指令
	send(fsd,"16",2,0); 		//二级目录（查询、退出）

	while(1)
	{
		bzero(buf,sizeof(buf));
		res = recv(fsd,buf,sizeof(buf),0);
		if(res < 0)
		{
			ERR_R("recv");
			break;
		}
		else if(res == 0)
		{
			//删除用户登录信息
			break;
		}
		switch(buf[0])
		{
		case '1':
			//查询函数
			do_select(db,fsd);
			//给客户端发送指令
			send(fsd,"16",2,0); 		//二级目录（查询、退出）
			break;
		case '2':
			//退出并删除登录信息
			printf("%s\n",name);
			goto END1;
			break;
		default :
			//给客户端发送提示
			send(fsd,"10",2,0); //输入错误
			break;
		}
	}
END1:
	close(fsd);
	printf("【%s:%d】客户端退出\n",inet_ntoa(prd.cin.sin_addr),ntohs(prd.cin.sin_port));
	pthread_exit(NULL);
}

//创建数据库并打开
sqlite3 *sqlite_db(int fsd)
{
	sqlite3 *db = NULL;
	if(sqlite3_open("./my.db",&db) != SQLITE_OK)
	{
		if(strcasecmp(sqlite3_errmsg(db),"unable to open database file") == 0)
		{
			fprintf(stderr,"__%d__ 库文件打开失败\n",__LINE__);
			return NULL;
		}
		else
		{
	
			fprintf(stderr,"__%d__ sqlite3_open:%s\n",__LINE__,sqlite3_errmsg(db));
			return NULL;
		}
	}
	char sq1[128] = "create table if not exists stu (id int primary key,word char,wordmeaning char)";
	char *errmsg  = NULL;

	if(sqlite3_exec(db,sq1,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		fprintf(stderr,"__%d__ sqlite3_exec:%s\n",__LINE__,errmsg);
		return NULL;
	}
	//通知客户端字典准备完毕
	send(fsd,"09",2,0); 		//字典数据库已准备完毕
	return db;
}

//创建套接字
int file_sfd()
{
	//创建套接字
	int sfd = socket(AF_INET,SOCK_STREAM,0);
	//填充结构体信息
	struct sockaddr_in sin;
	sin.sin_family  = AF_INET;
	sin.sin_port    = htons(8888);
	sin.sin_addr.s_addr = inet_addr("192.168.5.30");
	//允许端口快速重用
	int reuse =1;
	if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0)
	{
		printf("重用失败\n");
		return 0;
	}
	//绑定IP和端口信息
	if(bind(sfd,(struct sockaddr*)&sin,sizeof(sin)) <0)
	{
		printf("绑定失败\n");
		return -1;
	}
	//设为被动监听
	if(listen(sfd,10) < 0)
	{
		printf("监听失败\n");
		return -2;
	}
	printf("绑定成功，监听成功\n");
	return sfd;
}

//注册函数
int  user_name(sqlite3 *db,int fsd)
{
	int out = 0;
	char password[128] = "";
	char sql[128] = "";
	//创建用于保存注册用户的表
	char addr[128] = "create table if not exists stu1 (id char primary key,password char)";
	char *errmsg  = NULL;
	if(sqlite3_exec(db,addr,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		fprintf(stderr,"__%d__ sqlite3_exec:%s\n",__LINE__,errmsg);
		return -1;
	}
	//创建用于保存登录用户的表
	bzero(addr,sizeof(addr));
	sprintf(addr,"create table if not exists stu2 (id char primary key,password char)");
	if(sqlite3_exec(db,addr,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		fprintf(stderr,"__%d__ sqlite3_exec:%s\n",__LINE__,errmsg);
		return -1;
	}

loop:
	//清空数组
	bzero(sql,sizeof(sql));
	bzero(name,sizeof(name));
	bzero(password,sizeof(password));

	//给客户端发送提示
	send(fsd,"01",2,0); 		//请输你的用户名

	//接收客户端用户名
	recv(fsd,name,sizeof(name),0);

	//给客户端发送提示
	send(fsd,"02",2,0); 		//请输入登录密码

	//接收客户端用户密码
	recv(fsd,password,sizeof(password),0);

	//保存注册用户信息
	sprintf(sql,"insert into stu1 values(\"%s\",\"%s\");",name,password);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{

		//给客户端发送提示
		send(fsd,"08",2,0); 		//该用户已存在，请重新输入用户名
		//是否回到主页：【Y/N】
		//接收客户端用户密码
		recv(fsd,&out,1,0);
		if( (out == 'y') || (out=='Y') )
		{
			return 1;
		}
		goto loop;
	}

	//保存登录用户
	sprintf(sql,"insert into stu2 values(\"%s\",\"%s\");",name,password);
	if(sqlite3_exec(db,sql,NULL,NULL,NULL) != SQLITE_OK)
	{
		return -1;
	}

	//给客户端发送提示
	send(fsd,"13",2,0); 		//注册成功
	return 0;
}


//登录函数
int long_in_name(sqlite3 *db,int fsd)
{
	int row,colum;
	char all[128]  ="";
	char sql[128]  = "";
	char **result  = NULL;
	char password[128] = "";
loop:	
	//清空数组
	bzero(sql,sizeof(sql));
	bzero(name,sizeof(name));
	bzero(password,sizeof(password));

	//给客户端发送提示
	send(fsd,"01",2,0); 		//请输你的用户名

	//接收客户端用户名
	recv(fsd,name,sizeof(name),0);

	//给客户端发送提示
	send(fsd,"02",2,0); 		//请输入登录密码

	//接收客户端用户密码
	recv(fsd,password,sizeof(password),0);
	
	//根据用户名获取注册库中的密码
	sprintf(sql,"select * from stu1 where id=\"%s\";",name);
	if(sqlite3_get_table(db,sql,&result,&row,&colum,NULL) != SQLITE_OK)
	{

		//给客户端发送提示
		send(fsd,"03",2,0); 		//当前库中没有用户
		return -1; 		

	}

	//如果该用户未注册则执行线面这行语句
	if((row&&colum) == 0)
	{
		//给客户端发送提示
		send(fsd,"04",2,0); 		//该用户未注册
		return -1; 		
	}

	//密码匹配<<<重复登录判断>>>>
	if( (strcasecmp(result[2],name) != 0) && (strcasecmp(result[3],password) != 0) )
	{	
		//给客户端发送提示
		send(fsd,"05",2,0); 		//用户名/密码输入有误，请重新输入
		goto loop;
	}
	else
	{
		//判断用户是否重复登录
		sprintf(all,"insert into stu2 values(\"%s\",\"%s\");",name,password);
		if(sqlite3_exec(db,all,NULL,NULL,NULL) != SQLITE_OK)
		{
			//给客户端发送提示
			send(fsd,"06",2,0); 		//重复登录
			return -1;	
		}
	}

	//给客户端发送提示
	send(fsd,"07",2,0); 			//请输入登录密码
	return 0; 			

}

//查询函数
int do_select(sqlite3 *db,int fsd)
{
	char sql[128] ="";
	char addr[128] ="";
	char *errmsg = NULL;
	int row,colum;
	char **result = NULL;
	char out=0;
	int i=0,j=0;
	int index=3,a=0;
	while(1)
	{
		index = 3;
		bzero(addr,sizeof(addr));

		//给客户端发送提示
		send(fsd,"11",2,0); 		//提示客户端输入要查询的单词

		//接收客户端单词
		recv(fsd,addr,sizeof(addr),0);

		//根据单词查找词义
		sprintf(sql,"select * from stu where word=\"%s\";",addr);
		if(sqlite3_get_table(db,sql,&result,&row,&colum,&errmsg) != SQLITE_OK)
		{
			fprintf(stderr,"__%d__ sqlite3_get_table:%s\n",__LINE__,errmsg);
			return -1;
		}
		if(row&&colum)
		{
				for(j=0;j<colum;j++)
				{
					send(fsd,result[index],strlen(result[index]),0);
					send(fsd,"\t",1,0);
					index++;
				}
				send(fsd,"\n",1,0);
		}
		else
		{
			//给客户端发送提示
			send(fsd,"12",2,0);
		}
		//接收客户端指令
		recv(fsd,&out,2,0); 			//继续查询按回车,退出输入【q/Q】
		if( (out == 'q') || (out=='Q') )
		{
			break;
		}
	}
	//释放查询结果
	sqlite3_free_table(result);
	//通知客户端已退出查询
	send(fsd,"14",2,0);
	return 0;
}
