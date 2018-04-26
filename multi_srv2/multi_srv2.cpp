//
// Created by zwk on 18-4-25.
//
#include <iostream>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>  //memset
#include <arpa/inet.h>  //sockaddr_in htons
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include "ThreadPool.h"
#include "multi_srv2.h"

#define ERR_EXIT(m) \
     do \
      { \
        perror(m); \
      }while(0) \

using std::cout;
using std::endl;
using std::vector;

struct sockaddr_in srvaddr1;
struct sockaddr_in srvaddr2;

int anotherSrv;
//设置非阻塞模式
void setNonblock(int sock)
{
    //通过fcntl可以改变已打开的文件性质
    int flag=fcntl(sock,F_GETFL);
    if(flag==-1) ERR_EXIT("fcntl");
    flag=flag|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,flag)<0) ERR_EXIT("fcntl");
}




int main()
{
    cout<<"服务器2端口号和地址： "<<SRV2_PORT<<"-"<<SRV2_ADDR<< endl;
    int listenfd;//监听套接字
    if((listenfd=socket(AF_INET,SOCK_STREAM,0))<0)
        ERR_EXIT("socket");
    setNonblock(listenfd);
    memset(&srvaddr2,0,sizeof(srvaddr2));
    srvaddr2.sin_family=AF_INET;
    srvaddr2.sin_port=htons(SRV2_PORT);
    srvaddr2.sin_addr.s_addr=inet_addr(SRV2_ADDR);

    int on=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));//设置time wait？
    if(bind(listenfd,(sockaddr *)&srvaddr2,sizeof(srvaddr2 ))<0)
        ERR_EXIT("bind");
    if(listen(listenfd,SOMAXCONN)<0) //SOMAXCONN最大连接数 128
        ERR_EXIT("listen");

    //   sleep(5);
    //与另一个服务器连接
    if((anotherSrv=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
        ERR_EXIT("Srv1 socket");
    memset(&srvaddr1,0,sizeof(srvaddr1));
    srvaddr1.sin_family=AF_INET;
    srvaddr1.sin_port=htons(SRV1_PORT);
    srvaddr1.sin_addr.s_addr=inet_addr(SRV1_ADDR);

    if((connect(anotherSrv,(sockaddr *)&srvaddr1,sizeof(srvaddr1)))<0)
        cout<<"connect srv1 failed"<<endl;

    //创建epoll监听复用
    int epollfd;
    int nready;
    if((epollfd=epoll_create(maxEpoll))<0)
        ERR_EXIT("epoll create");
    struct epoll_event event;
    event.data.fd=listenfd;
    event.events=EPOLLIN||EPOLLET;//有数据到来||边沿触发
    epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&event);
    epoll_event eventList[20] = {0};

    //客户端的地址
    struct sockaddr_in cliaddr;
    socklen_t clilen=sizeof(cliaddr);
    int conn; //连接套接字

    //初始化线程池
    ThreadPool threadpool(3,5);

    while(1)
    {
        nready=epoll_wait(epollfd,eventList,maxEpoll,waitMaxTime);
        if(nready==-1)
        {
            if(errno==EINTR) continue;//慢系统永久阻塞当捕获到某个信号且相应信号处理函数返回时，这个系统调用被中断，调用返回错误，设置errno为EINTR
            cout<<"epoll wait"<<endl;
            cout << errno << endl;
        }
        if(nready==0) continue;

        //处理epoll中有信号的文件描述符
        for(int i=0;i<nready;i++)
        {

            //监听到有来连接，接受连接，并加入到epoll_ctl中
            if(eventList[i].data.fd==listenfd)
            {
                if((conn=accept(listenfd,(sockaddr *)&cliaddr,&clilen))<0)
                    ERR_EXIT("accpet");
                char *tmpAddr=inet_ntoa(cliaddr.sin_addr);
                int tmpPort=cliaddr.sin_port;    //不用转字节序？
                cout<<"srv1 recv a connet from:"<<tmpAddr<<":"<<tmpPort<<endl;

                //epoll ET模式必须非阻塞模式，
                setNonblock(conn);
                event.data.fd=conn;
                event.events=EPOLLIN||EPOLLET;
                epoll_ctl(epollfd,EPOLL_CTL_ADD,conn,&event);
            }
                //监听到可读事件
            else if(eventList[i].events &EPOLLIN)
            {
                conn=eventList[i].data.fd;
                if(conn<0) continue;
                Task newtask(conn);
                threadpool.addTask(newtask);
            }
            //监听到可写事件,数据发送
        }
    }
}







