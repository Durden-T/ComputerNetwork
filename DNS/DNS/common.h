#ifndef COMMON_H
#define COMMON_H


#include<iostream>
#include<fstream>
#include<unordered_map>
#include<boost/asio.hpp>
using namespace boost::asio;
using namespace std;

const unsigned short PORT(53);
const string DEFAULT_DNS("202.106.0.20");
const string DEFAULT_FILE("dnsrelay.txt");
const size_t BUF_SIZE(512);
const size_t HEAD_LEN(12);
const string BLOCKED("0.0.0.0");
const unsigned short MAX_ID(65535);
#define IDT_MAX_SIZE 1000
#define IDT_HALF_SIZE 500
class IpDomainNode
{
public:
	string ip;
	string domain;

	void set(string pip, string pdomain)
	{
		ip = pip;
		domain = pdomain;
	}
};

//ID转换表
//中继DNS请求包更名前的相关信息
class IDTransferNode
{
public:
	//旧ID
	unsigned short oldID;

	//请求者套接字地址
	ip::udp::endpoint ep;

	//标记是否已经完成解析
	bool processed;
};




#endif