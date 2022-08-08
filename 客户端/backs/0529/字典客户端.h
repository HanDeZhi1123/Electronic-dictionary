#ifndef __COM_H_
#define __COM_H_

//准备函数
int open_sock();

//一级目录
void one_dir();


//二级目录
void two_dir();

//发送数据
int  recv_input(int sfd);

//接收数据
void  send_out(int sfd);

//功能提示函数
void input_all(char buf[]);
#endif
