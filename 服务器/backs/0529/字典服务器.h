#ifndef __CONNECT__H_
#define __CONNECT__H_

#define PORT 8888
//#define IP  "192.168.5.11"

#define ERR_R(msg)  do{\
	fprintf(stderr,"__%d___:",__LINE__);\
	perror(msg);}while(0);
struct msg
{
	int newfd;
	struct sockaddr_in cin;
};

//创建数据库及打开数据库
sqlite3 *sqlite_db(int fsd);

//分支线程
void* each_other(void* arg);

//准备工作
int sfd_fd();


//查询函数
int do_select(sqlite3 *db,int fsd,char name_long_in[]);

//注册函数
int  user_name(sqlite3 *db,int fsd);

//登录函数
int long_in_name(sqlite3 *db,int fsd);

//退出函数删除登录用户信息
void long_out(char name_long_in[],sqlite3 *db);

//历史记录查询函数
void name_cat(sqlite3 *db,char name_long_in[],int fsd);

#endif
