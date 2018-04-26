//
// Created by zwk on 18-4-26.
//

#include <pthread.h>
#include "Condition.h"


int Condition::lock()
{
    return pthread_mutex_lock(&mutex);  //调用互斥锁的加锁函数进行加锁
}
int Condition::unlock()
{
    return pthread_mutex_unlock(&mutex);  //调用互斥锁的解锁函数进行解锁
}
int Condition::wait()
{
    return pthread_cond_wait(&cond,&mutex);  //调用条件变量的
}
int Condition::timedwait(timespec waittime)
{
    return pthread_cond_timedwait(&cond,&mutex,&waittime);  //调用条件变量的时间等待函数
}
int Condition::signal()
{
    return pthread_cond_signal(&cond);  //调用条件变量的信号发送函数
}
int Condition::broadcast()
{
    return pthread_cond_broadcast(&cond);  //调用条件变量的所有等待线程信号发送函数
}
int Condition::init()
{
    int status;
    if((status=pthread_mutex_init(&mutex,NULL)))//返回值不为0,初始化失败
        return status;
    if((status=pthread_cond_init(&cond,NULL)))//返回值不为0,初始化失败
        return status;
    return 0;
}
int Condition::destroy()
{
    int status;
    if((status=pthread_mutex_destroy(&mutex)))//返回值不为0,销毁失败
        return status;
    if((status=pthread_cond_destroy(&cond)))//返回值不为0,销毁失败
        return status;
    return 0;
}



