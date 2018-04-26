/*
 * dataSource.h
 *
 *  Created on: Apr 23, 2018
 *      Author: zwk
 */

#ifndef DATASOURCE_H_
#define DATASOURCE_H_
#define SRV1_PORT 2233
#define SRV1_ADDR "127.0.0.1"
#define SRV2_PORT 3322
#define SRV2_ADDR "127.0.0.2"

const int DEAL_DATA=0;//成交数据
const int SUB_DATA=1;//订阅数据
const int DATA_FROM_SOURCE=0;  //数据源的数据
const int DATA_FROM_USER=1; //客户的请求
const int DATA_FROM_SRV=2;  //其他服务器数据，转发给客户端
const int DATA_FROM_USER_RECONN=3; //某个客户端发来的重新连接的消息，删掉该客户端之前的订阅号，fd清0
struct packageHead
{
    int flag;//标志位
    unsigned short dataLen; //数据正文长度
};

//交易数据
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




#endif /* DATASOURCE_H_ */