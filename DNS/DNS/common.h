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

//IDת����
//�м�DNS���������ǰ�������Ϣ
class IDTransferNode
{
public:
	//��ID
	unsigned short oldID;

	//�������׽��ֵ�ַ
	ip::udp::endpoint ep;

	//����Ƿ��Ѿ���ɽ���
	bool processed;
};




#endif