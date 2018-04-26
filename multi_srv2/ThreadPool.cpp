//
// Created by zwk on 18-4-26.
//


#include <iostream>
#include <time.h>
#include "ThreadPool.h"
#include <errno.h>
#include <cstdlib>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>

using std::cout;
using std::endl;
const time_t maxWaitTime=10;
extern  int anotherSrv;
extern sockaddr_in srvaddr2;


void *threadRoutine(void* arg);

//为防止粘包，采取定长包方式
int recvPack(int sockfd,char *buf,int len )
{
    int leftlen=len;//剩余长度
    char* p=buf;
    int recvlen;
    int ret=0;
    if(buf==NULL)
        return -2;
    while(leftlen>0)
    {
        recvlen=recv(sockfd,p,leftlen, MSG_NOSIGNAL);
        if(recvlen<=0)
        {
            if(errno==EAGAIN)  //由于是非阻塞的模式,所以当errno为EAGAIN时,表示当前缓冲区已无数据可读,在这里就当作是该次事件已处理过。
                break;
            else if(recvlen<0)
            {
                cout<<"读取错误,对方关闭"<<endl;
                close(sockfd);
            }
            break;
//    		return recvlen;
        }
        p=p+recvlen;//缓冲区指针前移
        leftlen=leftlen-recvlen;//剩余长度变短
        ret=ret+recvlen;
    }
    return ret;
}


void taskRun(Task task,ThreadPool& threadpool)
{

    int sockfd=task.sock;



    packageHead Head;
    memset(&Head, 0, sizeof(Head));
    int ret = recvPack(sockfd, (char *) &Head, sizeof(Head));
    if (ret < ((int) sizeof(Head))) //数据长度不够
        return;
    //数据源传来数据
    if (Head.flag == DATA_FROM_SOURCE)
    {
        DealData dealData;
        memset(&dealData, 0, sizeof(dealData));
        int dealRet = recvPack(sockfd, (char *) &dealData, sizeof(dealData));
        if(dealRet<(int) sizeof(DealData))//未读到完整数据
            return;
        cout << "读取包体大小：" << dealRet << endl;
        cout << "包体的ID:" << dealData.userId << endl;

        //转发给用户
        threadpool.cond.lock();

        auto it=threadpool.userMap[dealData.userId].begin(); //map的set
        for(;it!=threadpool.userMap[dealData.userId].end();it++)
        {
            send(*it,&dealData,sizeof(DealData),MSG_NOSIGNAL);//set存的是订阅了该ID的fd?
            cout<<"服务器1转发数据源数据成功发送给用户"<<endl;
        }
        threadpool.cond.unlock();

        //转发给另外一个服务器，构建新的包头和数据体
        if(anotherSrv<0)
            return; //之前连接另一个服务器失败
        packageHead srvHead;
        srvHead.flag=DATA_FROM_SRV;
        srvHead.dataLen=(unsigned short)(sizeof(DealData));
        char *sendData=new char[sizeof(srvHead)+sizeof(DealData)];
        memset(sendData,0,sizeof(srvHead)+sizeof(DealData));
        memcpy(sendData,&srvHead,sizeof(srvHead));
        memcpy(sendData+sizeof(srvHead),&dealData,sizeof(DealData));

        int srvRet=send(anotherSrv,sendData,sizeof(srvHead)+sizeof(DealData),MSG_NOSIGNAL);//another是一个此客户端connect另一个，而不是accpept,所以可以发
        ///发送失败，重新连接
        delete[] sendData;//注意销毁
        if(srvRet==-1)
        {
            close(anotherSrv);
            if((anotherSrv=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
                perror("创建anotherSrv失败");
            if((connect(anotherSrv,(sockaddr *)&srvaddr2,sizeof(srvaddr2)))<0)
                cout<<"connect srv2失败"<<endl;
        }
        if(srvRet>0)
        {
            cout<<"成功发送给服务器2"<<endl;
        }


    }

        //客户端的请求
    else if(Head.flag==DATA_FROM_USER)
    {
        //memset(data,0,1024);
        char name[7];
        memset(name,0,7);
        ret=recvPack(sockfd,name,Head.dataLen);
        if(ret<=0)
            return;

        //除去\0，6位是真实ID
        if(strlen(name)==6)
        {
            string tmp(name);
            threadpool.cond.lock();
            threadpool.userMap[tmp].insert(sockfd);
            threadpool.cond.unlock();
            cout<<"用户："<<tmp<<"请求了数据"<<endl;
            cout<<"user's:sockfd="<<*threadpool.userMap[tmp].begin()<<endl;
        }
    }


        //另一个服务器发来的数据
    else if(Head.flag==DATA_FROM_SRV)
    {
        DealData dealData;
        memset(&dealData, 0, sizeof(dealData));
        int dealRet = recvPack(sockfd, (char *) &dealData, sizeof(dealData));
        if(dealRet<(int) sizeof(DealData))//未读到完整数据
            return;
        cout<< "收到来自其他服务器的数据"<<endl;
        threadpool.cond.lock();
        auto it=threadpool.userMap[dealData.userId].begin(); //map的set
        for(;it!=threadpool.userMap[dealData.userId].end();it++)
        {
            send(*it,&dealData,sizeof(DealData),MSG_NOSIGNAL);
            cout<<"服务器1转发其他服务器数据成功发送给用户"<<endl;

        }
        threadpool.cond.unlock();

    }
        //客户端重连信号，之前的fd清0
    else if(Head.flag==DATA_FROM_USER_RECONN)
    {
        cout<<"重新连接"<<endl;
        threadpool.cond.lock();
        auto it=threadpool.userMap.begin(); //map的set
        for(;it!=threadpool.userMap.end();it++)
        {
            it->second.erase(sockfd);
        }
        threadpool.cond.unlock();
    }


}


//初始化线程池
void ThreadPool::initThreadPool()
{
    pthread_t id=0;
    int ret=0;
    this->cond.lock();//创建线程时要加锁
    for(int i=0;i<this->initThreadNum;i++)
    {
        ret=pthread_create(&id,NULL,&threadRoutine,(void *)this);//id会引用改成现在的id值
        if(ret==0)//返回0,线程创建成功，线程ID入队
        {
            this->threadQueue.push(id);
            this->nowThreadNum++;
        }
    }
    this->cond.unlock();
}
void ThreadPool::destroyThreadPool()//销毁线程池
{
    this->cond.lock();
    if(!isDestroy)
        isDestroy=true;
    if(this->idleThreadNum>0)
        this->cond.broadcast();
    if(this->nowThreadNum>0)
    {
        while(this->nowThreadNum>0)
        {
            this->cond.wait();
        }
    }
    //根据线程id销毁
    while(!this->threadQueue.empty())
    {
        pthread_join(this->threadQueue.front(),nullptr);
        this->threadQueue.pop();
    }


    this->cond.unlock();
}

void *threadRoutine(void* arg)//线程入口函数
{
    ThreadPool *threadpool=(ThreadPool *)arg;
    timespec timeOut = {0};
    int flag=1;
    while(flag)//这个循环什么时候退出？
    {
        threadpool->cond.lock();

        //创建了一个新线程，空闲+1
        (threadpool->idleThreadNum)++;
        //等到任务到来或者销毁通知
        while(threadpool->taskQueue.empty()&& !threadpool->isDestroy)
        {
//            cout<<"Thread:"<<(int)pthread_self()<<"is waiting"<<endl;
            clock_gettime(CLOCK_REALTIME, &timeOut); //设置超时时间
            timeOut.tv_sec+=maxWaitTime;
            if(threadpool->cond.timedwait(timeOut)==ETIMEDOUT)
            {
                cout<<"Thread"<<pthread_self() << " waiting timeout." << endl;
                flag=0;
                break; //超时退出
            }
//            sleep(1);
        }


        (threadpool->idleThreadNum)--;

        //如果任务队列有任务，进行处理
        if(!threadpool->taskQueue.empty())
        {
            Task task=threadpool->taskQueue.front();
            threadpool->taskQueue.pop();//取出第一个任务,弹出

            //任务执行时可以解锁，让其他线程操作
            threadpool->cond.unlock();
            taskRun(task,(*threadpool));
            threadpool->cond.lock();
        }

        //如果是销毁信号，并且任务队列为空
        if(threadpool->isDestroy && threadpool->taskQueue.empty())
        {
            threadpool->nowThreadNum--;
            if(threadpool->nowThreadNum<=0)
                threadpool->cond.signal();
            flag=0;
        }
        threadpool->cond.unlock();
    }

    //退出线程
    cout << "Thread " << pthread_self() << " exited." << endl;
    return arg;
}
void ThreadPool::addTask(Task task) //添加任务
{
    this->cond.lock();
    this->taskQueue.push(task);
    if(this->idleThreadNum>0)//有空闲线程，给等待线程发出唤醒信号
    {
        this->cond.broadcast();
    }
    else//没有空闲线程，看情况添加线程
    {
        if(this->nowThreadNum<this->maxThreadNum)
        {
            pthread_t tid;
            int ret=pthread_create(&tid,nullptr,&threadRoutine,(void *)(this));
            if(ret==0)
            {
                this->threadQueue.push(tid);
                this->nowThreadNum++;
            }
        }
    }
    this->cond.unlock();
}



