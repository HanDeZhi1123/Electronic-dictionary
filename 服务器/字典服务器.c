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
#include<time.h>
#include"字典服务器.h"



char name_in[128] =""; 	//全局变量用于存储连接时的用户名

//准备工作
int sfd_fd()
{
	system("ifconfig"); 		//获取本机IP打印在终端上
#if 1
	char buf[20] = "\0";
	//输入ifconfig查询到的本机IP
	printf("端口8888，请输入你要绑定服务器的IP>>>>");   
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
	//sin.sin_addr.s_addr = inet_addr(IP);
	sin.sin_addr.s_addr = inet_addr(buf);
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
	int rese = 0;
	int out = 1; 	//退出一级目录循环条件标志位
	char buf[128] = "";
	char name_long_in[20] = "";
	struct msg prd = *(struct msg*)arg;
	int fsd = prd.newfd;

	bzero(buf,sizeof(buf));
	bzero(name_long_in,sizeof(name_long_in));
	//分离线程
	pthread_detach(pthread_self());

	printf("【%s:%d】客户端已连接\n",inet_ntoa(prd.cin.sin_addr),ntohs(prd.cin.sin_port));

	//创建数据库并打开
	sqlite3 *db = sqlite_db(fsd);

loop_in:
	//给客户端发送指令
	send(fsd,"15",2,0); 		//一级目录（注册、登录、退出）
	do{
		out = 1;
		bzero(buf,sizeof(buf));
		res = recv(fsd,buf,sizeof(buf),0);
		if((res < 0) || (res == 0))
		{
			goto END1;
		}
		switch(buf[0])
		{
		case '1' :
			//注册函数
			rese = user_name(db,fsd);
			if(rese < 0)
			{
				goto loop_in;
			}
			else if(rese == 1)
			{
				goto END1;
			}
			strcpy(name_long_in,name_in);
			out = 0;
			break;
		case '2' :
			//登录函数
			rese = long_in_name(db,fsd); 
			if(rese < 0)
			{
				goto loop_in;
			}
			else if(rese == 1)
			{
				goto END1;
			}
			strcpy(name_long_in,name_in);
			out = 0;
			break;
		case '3' :
			//退出
			goto END1;
			break;
		default:
			send(fsd,"10",2,0);
			out = 1;
			break;
		}
	}while(out);
all:
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
			break;
		}
		switch(buf[0])
		{
		case '1':
			//查询函数
			rese = do_select(db,fsd,name_long_in);
			if(rese < 0)
			{
				goto all;
			}
			else if(rese == 1)
			{
				goto END1;
			}
			goto all;
			break;
		case '2':
			//退出并删除登录信息
			goto END1;
			break;
		case '3':
			//返回上一级并删除登录信息
			long_out(name_long_in,db);
			printf("用户%s>>>【%s:%d】客户端退出\n",name_long_in,\
					inet_ntoa(prd.cin.sin_addr),ntohs(prd.cin.sin_port));
			goto loop_in;
			break;
		case '4':
			//历史记录查询
			name_cat(db,name_long_in,fsd);
			send(fsd,"0#",2,0); 		//输入任意字符清屏
			bzero(buf,sizeof(buf));
#if 0
			res = recv(fsd,buf,sizeof(buf),0);
			if(res < 0)
			{
				ERR_R("recv");
				break;
			}
			else if(res == 0)
			{
				break;
			}
#endif
			goto all;
			break;
		default :
			//给客户端发送提示
			send(fsd,"10",2,0); //输入错误
			break;
		}
	}
END1:
	//删除用户登录信息
	long_out(name_long_in,db);
	close(fsd);
	printf("用户%s>>>【%s:%d】客户端退出\n",name_long_in,\
			inet_ntoa(prd.cin.sin_addr),ntohs(prd.cin.sin_port));
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

	//创建用于保存注册用户的表
	char addr[128] = "create table if not exists stu1 (id char primary key,password char)";
	if(sqlite3_exec(db,addr,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		fprintf(stderr,"__%d__ sqlite3_exec:%s\n",__LINE__,errmsg);
		return NULL;
	}
	//创建用于保存登录用户的表
	bzero(addr,sizeof(addr));
	sprintf(addr,"create table if not exists stu2 (id char primary key,password char)");
	if(sqlite3_exec(db,addr,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		fprintf(stderr,"__%d__ sqlite3_exec:%s\n",__LINE__,errmsg);
		return NULL;
	}
	//创建保存单词查询历史记录的表
	bzero(addr,sizeof(addr));
	sprintf(addr,"create table if not exists stu3 (name char,word char ,wordmeaning char,time char)");
	if(sqlite3_exec(db,addr,NULL,NULL,&errmsg) != SQLITE_OK)
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
	ssize_t res = 0;
	char out = 0;
	char password[128] = "\0";
	char name[128] = "\0";
	char sql[128] = "\0";
	char *errmsg  = NULL;

loop:
	//清空数组
	bzero(sql,sizeof(sql));
	bzero(name,sizeof(name));
	bzero(name_in,sizeof(name_in));
	bzero(password,sizeof(password));

	//给客户端发送提示
	send(fsd,"01",2,0); 		//请输你的用户名

	//接收客户端用户名
	res = recv(fsd,name_in,sizeof(name_in),0);
	if(res == 0)
	{
		return 1;
	}
	strcpy(name,name_in);

	//给客户端发送提示
	send(fsd,"02",2,0); 		//请输入登录密码

	//接收客户端用户密码
	res = recv(fsd,password,sizeof(password),0);
	if(res == 0)
	{
		return 1;
	}

	//保存注册用户信息stu1
	sprintf(sql,"insert into stu1 values(\"%s\",\"%s\");",name,password);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{

		//给客户端发送提示
		send(fsd,"08",2,0); 		//该用户已存在，请重新输入用户名
		//是否回到主页：【Y/N】
		//接收客户端用户指令
		res = recv(fsd,&out,1,0);
		if(res == 0)
		{
			return 1;
		}
		if( (out == 'y') || (out=='Y') )
		{
			return -1;
		}
		goto loop;
	}

	//保存登录用户stu2
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
	ssize_t res = 0; 				//用于判断客户端有没有断开
	int row,colum;
	char out_in=0; 					//功能选择
	char all[128]  ="\0"; 			//判断是否重复登录的指令临时存储
	char seva[128]  ="\0"; 			//
	char sql[128]  = "\0"; 			//发送的指令临时存储
	char name[128] = "\0"; 			//用户的名字
	char **result  = NULL;
	char password[128] = "\0"; 		//用户的密码
	int falg = 3; 					//密码输入错误次数限制

	//给客户端发送提示
	send(fsd,"0@",2,0); 		//密码错误3次自动返回主页

loop:	
	//清空数组
	bzero(sql,sizeof(sql));
	bzero(seva,sizeof(seva));
	bzero(name,sizeof(name));
	bzero(name_in,sizeof(name_in));
	bzero(password,sizeof(password));

	//给客户端发送提示
	send(fsd,"01",2,0); 		//请输你的用户名

	//接收客户端用户名
	res = recv(fsd,name_in,sizeof(name_in),0);
	if(res == 0)
	{
		return 1;
	}
	strcpy(name,name_in);

	//给客户端发送提示
	send(fsd,"02",2,0); 		//请输入登录密码

	//接收客户端用户密码
	res = recv(fsd,password,sizeof(password),0);
	if(res == 0)
	{
		return 1;
	}

	//根据用户名获取注册库中的密码
	sprintf(sql,"select * from stu1 where id=\"%s\";",name);
	if(sqlite3_get_table(db,sql,&result,&row,&colum,NULL) != SQLITE_OK)
	{

		//给客户端发送提示
		if(send(fsd,"03",2,0) == -1) 		//当前库中没有用户
		{
			return -1;
		}
		return -1; 		

	}

	//如果该用户未注册则执行线面这行语句
	if((row&&colum) == 0)
	{
		//给客户端发送提示
		if(send(fsd,"04",2,0) == -1) 	//该用户未注册
		{
			return -1;
		}
		return -1; 		
	}

	//密码匹配
	if(strcasecmp(result[3],password) != 0)
	{
		if(falg != 1)
		{
			falg--;
			//给客户端发送提示
			if(send(fsd,"05",2,0) == -1) 		//用户名/密码输入有误，请重新输入
			{
				return -1;
			}
			goto loop;
		}
		else
		{ 
			//密码输入次数上限、自动回到主页（注册）
			if(send(fsd,"17",2,0) == -1) 		//提示密码次数已达上限
			{
				return -1;
			}
			return -1;
		}
	}

	//判断用户是否重复登录
	bzero(sql,sizeof(sql));
	sprintf(sql,"insert into stu2 values(\"%s\",\"%s\");",name,password);
	if(sqlite3_exec(db,sql,NULL,NULL,NULL) != SQLITE_OK)
	{
		//给客户端发送提示
		if(send(fsd,"06",2,0) == -1)		//重复登录,是否回到主页【y/n】
		{
			return -1;
		}
		res = recv(fsd,&out_in,1,0); 		//接收用户端指令
		if(res == 0)
		{
			return 1;
		}
		if( (out_in == 'y')  || (out_in == 'Y'))
		{
			return -1;
		}
		goto loop;
	}

	//给客户端发送提示
	if(send(fsd,"07",2,0) == -1)			//登录成功
	{
		return -1;
	}
	return 0; 			

}

//查询函数
int do_select(sqlite3 *db,int fsd,char name_long_in[])
{
	ssize_t res = 0; 			//判断用户端退出
	char sql[128] =""; 			//发送指令（把单词送到数据库中查找）
	char addr[128] =""; 		//接收客户端单词
	char buf[128] = ""; 		//保存要查询到的单词
	char seva[128] = ""; 		//保存查询记录
	char *errmsg = NULL;
	int row,colum;
	char **result = NULL;
	char out=0;  				//用于判断用户是否需要继续查询
	int i=0; 					//用于send停止发送的条件
	int index=3; 				//下标，决定发送的是什么数据
	struct tm* time_cat;
	time_t t = 0; 				//存储时间
	while(1)
	{
		index = 3;

		//给客户端发送提示
		if(send(fsd,"11",2,0) == -1) 	//提示客户端输入要查询的单词
		{
			return -1;
		}
		//接收客户端单词
		bzero(addr,sizeof(addr));
		res = recv(fsd,addr,sizeof(addr),0);
		if(res == 0)
		{
			return 1;
		}

		//根据单词查找词义
		bzero(sql,sizeof(sql));
		sprintf(sql,"select * from stu where word=\"%s\";",addr);
		if(sqlite3_get_table(db,sql,&result,&row,&colum,&errmsg) != SQLITE_OK)
		{
			fprintf(stderr,"__%d__ sqlite3_get_table:%s\n",__LINE__,errmsg);
			return -1;
		}
		//把查询到的数据发送给客户端
		if(row&&colum)
		{
			for(i=0;i<row;i++)
			{
				bzero(buf,sizeof(buf));
				sprintf(buf,"%s%c%s%c%s",result[(index)],9,\
						result[(index+1)],9,result[(index+2)]);
				if(send(fsd,buf,sizeof(buf),0) == -1)
				{
					return -1;
				}
			}
		}
		else
		{
			//给客户端发送提示
			if(send(fsd,"12",2,0) == -1) 		//未找到单词
			{
				return -1;
			}
		}
		if(row&&colum)
		{
			//保存登录用户查询历史记录stu3
			bzero(seva,sizeof(seva));
			time(&t);
			time_cat = localtime(&t);
			sprintf(seva,"insert into stu3 values(\"%s\",\"%s\",\"%s\",\"%d月  %d日  %02d:%02d\");",\
					name_long_in,result[4],result[5],time_cat->tm_mon+1,time_cat->tm_mday,\
					time_cat->tm_hour,time_cat->tm_min);
			if(sqlite3_exec(db,seva,NULL,NULL,NULL) != SQLITE_OK)
			{
				return -1;
			}
		}
		//是否退出查询
		if(send(fsd,"0*",2,0)== -1)
		{
			return -1;
		}
		//接收客户端指令
		res = recv(fsd,&out,1,0); 			//继续查询按回车,退出输入【q/Q】
		if(res == 0)
		{
			return 1;
		}
		if( (out == 'y') || (out=='Y') )
		{
			break;
		}
	}
	//释放查询结果
	sqlite3_free_table(result);
	//通知客户端已退出查询
	if(send(fsd,"14",2,0) == -1)
	{
		return -1;
	}
	return 0;
}

//退出函数删除登录用户信息
void long_out(char name_long_in[],sqlite3 *db)
{
	char buf[128] ="\0";
	char sql[128] = "\0";

	//保存登录用户
	sprintf(sql,"delete from stu2 where id=\"%s\" ;",name_long_in);
	if(sqlite3_exec(db,sql,NULL,NULL,NULL) != SQLITE_OK)
	{
		return ;
	}
	return ;
}

//历史记录查寻
void name_cat(sqlite3 *db,char name_long_in[],int fsd)
{
	char sql[128] = "\0";
	char buf[128] = "\0";
	int i,j;
	int index = 0;
	int row,colum;
	char *errmsg = NULL;
	char **result = NULL;

	bzero(sql,sizeof(sql));

	//根据单词查找词义                                                    
	sprintf(sql,"select * from stu3 where name=\"%s\";",name_long_in);
	if(sqlite3_get_table(db,sql,&result,&row,&colum,&errmsg) != SQLITE_OK)
	{
		fprintf(stderr,"__%d__ sqlite3_get_table:%s\n",__LINE__,errmsg);
		return ;
	}
	if(row&&colum)
	{
		for(i=0;i<=row;i++)
		{
			sprintf(buf,"%s%c%s%c%s%c%c%s",result[(index)],9,\
					result[(index+1)],9,result[(index+2)],9,9,result[(index+3)]);
			if(send(fsd,buf,sizeof(buf),0) == -1)
			{
				return;
			}
			bzero(buf,sizeof(buf));
			index = index+4;
		}
	}
	else
	{
		//提示客户端未找到查询历史记录
		if(send(fsd,"18",2,0) == -1)
		{
			return;
		}
		return;
	}
	send(fsd,"19",2,0); 		//提示客户端历史记录查询完毕
	return;
}
