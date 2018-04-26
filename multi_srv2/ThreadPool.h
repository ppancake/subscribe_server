//
// Created by zwk on 18-4-26.
//

#ifndef MULTI_SRV2_THREADPOOL_H
#define MULTI_SRV2_THREADPOOL_H

#include<pthread.h>
#include<string>
#include<queue>
#include<set>
#include <unordered_map>
#include "Condition.h"
using std::string;
using std::queue;
using std::set;
using std::unordered_map;

const int DATA_FROM_SOURCE=0;  //数据源的数据
const int DATA_FROM_USER=1; //客户的请求
const int DATA_FROM_SRV=2;  //其他服务器数据，转发给客户端
const int DATA_FROM_USER_RECONN=3; //某个客户端发来的重新连接的消息，删掉该客户端之前的订阅号，fd清0
const int MAX_EVENT_NUMBER=200;

struct packageHead
{
    int flag;//标志位
    unsigned short dataLen; //数据正文长度
};

struct DealData
{
    char userId[7];              //客户号，6位
    char dealType;              //成交类型
    char ShareholderCode[11];   //股东代码，10位
    char StockCode[7];          //证券代码，6位
    int dealNum;                //成交数量
    double price;               //成交价格
    char orderNo[23];           //订单编号,22位
    char dealNo[23];            //成交编号,22位
    char dealDate[9];           //成交日期，8位
    char dealTime[7];           //成交时间，6位
    char serviceType;           //服务类型
};

class Task
{
public:
    Task(int fd) :sock(fd) {}
    void taskRun();
public:
    int sock;
};

class ThreadPool
{
public:
    void initThreadPool();//初始化线程池
    void destroyThreadPool();//销毁线程池
    friend void *threadRoutine(void* arg); //线程入口函数
    friend void taskRun(Task task,ThreadPool&);
    void addTask(Task task);//添加任务??之前放到析构函数后面，：：居然不显示？？
    ThreadPool(int initNum, int maxNum):initThreadNum(initNum),nowThreadNum(0),idleThreadNum(0),maxThreadNum(maxNum),isDestroy(false)
    {
        initThreadPool();
    }
    ~ThreadPool()
    {
        destroyThreadPool();
    }

private:
    Condition cond; //锁和条件变量
    int initThreadNum; //初始线程池数量
    int nowThreadNum; //当前线程池线程数量
    int idleThreadNum; //当前空闲线程数量
    int maxThreadNum; //最大允许的线程数量
    queue<Task> taskQueue; //任务队列
    queue<pthread_t> threadQueue; //创建线程的ID集合..或者采用vector/数组存储线程的id也行
    unordered_map<string, set<int>> userMap;
    bool isDestroy; //线程销毁通知
};




#endif //MULTI_SRV2_THREADPOOL_H
