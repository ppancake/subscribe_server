/*
 * multi_srv1.h
 *
 *  Created on: Apr 22, 2018
 *      Author: zwk
 */

#ifndef MULTI_SRV_H_
#define MULTI_SRV_H_
#include <queue>
#include <set>
#include <string>
using std::set;
using std::string;
using std::queue;
//包头
#define SRV1_PORT 2233
#define SRV1_ADDR "127.0.0.1"
#define SRV2_PORT 3322
#define SRV2_ADDR "127.0.0.2"


const int maxEpoll=500; //最大监听数
const int waitMaxTime=100*1000; //最大等待100s




#endif /* MULTI_SRV_H_ */