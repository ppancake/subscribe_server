/*
 * multi_cli.cpp
 *
 *  Created on: Apr 23, 2018
 *      Author: zwk
 */
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctime>
#include <iostream>
#include "multi_cli.h"
using std::cout;
using std::endl;

struct sockaddr_in srvaddr1;
struct sockaddr_in srvaddr2;
int main()
{
    User user{SUB_DATA,"123456"};
    int sockfd;
    if((sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
        perror("socket fail");
    srvaddr1.sin_family=AF_INET;
    srvaddr1.sin_port=htons(SRV1_PORT);
    srvaddr1.sin_addr.s_addr=inet_addr(SRV1_ADDR);

    srvaddr2.sin_family=AF_INET;
    srvaddr2.sin_port=htons(SRV2_PORT);
    srvaddr2.sin_addr.s_addr=inet_addr(SRV2_ADDR);

    //随机连接一个服务器
    int srvNo;
    srand((unsigned int)(time(nullptr)));  //随机种子
    int randomNo=rand()%2;
    int connErr;
    if(randomNo==1)
    {
        connErr=connect(sockfd,(struct sockaddr*)&srvaddr1,sizeof(srvaddr1));
        //连接失败换一个连接
        if(connErr==-1)
        {
            connErr=connect(sockfd,(struct sockaddr*)&srvaddr2,sizeof(srvaddr2));
            if(connErr!=-1)
                srvNo=2;
        }
        else if(connErr!=-1)
        {
            srvNo=1;
        }
    }
    else
    {
        connErr=connect(sockfd,(struct sockaddr*)&srvaddr2,sizeof(srvaddr2));
        //连接失败换一个连接
        if(connErr==-1)
        {
            connErr=connect(sockfd,(struct sockaddr*)&srvaddr1,sizeof(srvaddr1));
            if(connErr!=-1)
                srvNo=1;
        }
        else if(connErr!=-1)
        {
            srvNo=2;
        }
    }
    //2个都没连接成功
    if(connErr==-1)
    {
        perror("cannot connect to server:");
        return -1;
    }
    if(connErr!=0)
    {
        cout<<"连接成功至服务器"<<srvNo<<endl;
    }

    //开始发送数据
    char str[]="000001";
    packageHead head;
    head.flag=DATA_FROM_USER;
    head.dataLen=sizeof(str);
    char *sendData=new char[sizeof(head)+sizeof(str)];
    memset(sendData,0,sizeof(head)+sizeof(str));
    memcpy(sendData,&head,sizeof(head));
    memcpy(sendData+sizeof(head),&str,sizeof(str));
    int sendRet=send(sockfd,sendData,sizeof(head)+head.dataLen,MSG_NOSIGNAL);
    //发送失败，断开连接，连另一个
    if(sendRet==-1)
    {
        close(sockfd);
        if((sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
            perror("socket fail");
        if(srvNo==0)
        {
            srvNo=rand()%2;
        }
        if(srvNo==1)//之前连的服务器1
        {
            connErr=connect(sockfd,(struct sockaddr*)&srvaddr2,sizeof(srvaddr2));
            srvNo=2;
        }
        else if(srvNo==2)//之前连的服务器1
        {
            connErr=connect(sockfd,(struct sockaddr*)&srvaddr1,sizeof(srvaddr1));
            srvNo=1;
        }
        if(connErr!=-1)
            cout<<"连接至服务器："<<srvNo<<endl;
        else
            cout<<"无法连接至服务器"<<endl;
    }
    else
    {
        cout<<"发送成功"<<endl;
    }
    delete[] sendData;

    bool exit=false;
    while(!exit)
    {
        char buf[200];
        memset(buf,0,sizeof(buf));
        int recvLen=recv(sockfd,buf,200,0);
        if(recvLen==104)
        {
            DealData dealData;
            memset(&dealData,0,sizeof(DealData));
            memcpy(&dealData,buf,sizeof(DealData));
            printf("\n-------------------接收数据来自服务器%d-----------------\n",srvNo);
            cout<<"客户号："<<dealData.userId<<endl;
            cout<<"成交类型："<<dealData.dealType<<endl;
            cout<<"股东代码："<<dealData.ShareholderCode<<endl;
            cout<<"证券代码："<<dealData.StockCode<<endl;
            cout<<"成交数量："<<dealData.dealNum<<endl;
            cout<<"成交价格："<<dealData.price<<endl;
            cout<<"订单编号："<<dealData.orderNo<<endl;
            cout<<"成交编号："<<dealData.dealNo<<endl;
            cout<<"成交日期："<<dealData.dealDate<<endl;
            cout<<"成交时间："<<dealData.dealTime<<endl;
            cout<<"服务类型："<<dealData.serviceType<<endl;
        }
        if(recvLen==-1)//再换一个
        {
            close(sockfd);
            if((sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
                perror("socket fail");
            if(srvNo==0)
            {
                srvNo=rand()%2;
            }
            if(srvNo==1)//之前连的服务器1
            {
                connErr=connect(sockfd,(struct sockaddr*)&srvaddr2,sizeof(srvaddr2));
                srvNo=2;
            }
            else if(srvNo==2)//之前连的服务器1
            {
                connErr=connect(sockfd,(struct sockaddr*)&srvaddr1,sizeof(srvaddr1));
                srvNo=1;
            }
            if(connErr!=-1)
                cout<<"连接至服务器："<<srvNo<<endl;
            else
            {
                cout<<"无法连接至服务器"<<endl;
                perror("connect fail");
            }
        }
        sleep(1);
    }
    close(sockfd);
    return 0;
}

