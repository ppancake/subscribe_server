/*
 * dataSouce.cpp
 *
 *  Created on: Apr 23, 2018
 *      Author: zwk
 */

#include "dataSource.h"
#include <iostream>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

using std::cout;
using std::endl;

void routine()
{
    struct sockaddr_in srvaddr1;
    struct sockaddr_in srvaddr2;

    srvaddr1.sin_family=AF_INET;
    srvaddr1.sin_port=htons(SRV1_PORT);
    srvaddr1.sin_addr.s_addr=inet_addr(SRV1_ADDR);

    srvaddr2.sin_family=AF_INET;
    srvaddr2.sin_port=htons(SRV2_PORT);
    srvaddr2.sin_addr.s_addr=inet_addr(SRV2_ADDR);

    int sockfd;
    if((sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
        perror("socket创建");

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
//    if(connErr==-1)
//    {
//        perror("cannot connect to server:");
//        exit(0);
//    }
    if(connErr!=0)
    {
        cout<<"连接成功至服务器"<<srvNo<<endl;
    }

    //数据发送
    packageHead head;
    head.flag=DATA_FROM_SOURCE;
    head.dataLen=sizeof(DealData);
    DealData sourceData{"000001", '7', "A834454980", "004772", 4863, 34.76,
                        "debyh43920170706jdfis8", "lskdj94820170709dkdjf7", "20180423", "202020", '1'};

    char sendData[sizeof(head)+sizeof(DealData)];
    cout<<sizeof(head)+sizeof(DealData)<<endl;
    cout<<"客户号："<<sourceData.userId<<endl;
    cout<<"成交类型："<<sourceData.dealType<<endl;
    cout<<"股东代码："<<sourceData.ShareholderCode<<endl;
    cout<<"证券代码："<<sourceData.StockCode<<endl;
    cout<<"成交数量："<<sourceData.dealNum<<endl;
    cout<<"成交价格："<<sourceData.price<<endl;
    cout<<"订单编号："<<sourceData.orderNo<<endl;
    cout<<"成交编号："<<sourceData.dealNo<<endl;
    cout<<"成交日期："<<sourceData.dealDate<<endl;
    cout<<"成交时间："<<sourceData.dealTime<<endl;
    cout<<"服务类型："<<sourceData.serviceType<<endl;
    while(1)
    {
        memset(sendData,0,sizeof(head)+sizeof(DealData) + 1);
        memmove(sendData,&head,sizeof(head));
        memmove(sendData+sizeof(head),(char *)&sourceData,sizeof(DealData));

        int sendRet=write(sockfd,sendData,sizeof(head)+sizeof(DealData));
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
        sleep(5);
    }
    delete[] sendData;
    close(sockfd);
    pthread_detach(pthread_self());
}

int main()
{
    routine();
    return 0;
}

