#ifndef TOOLS_H
#define TOOLS_H



#include"common.h"


//�����������,�����������󷵻�false,���򷵻�true
bool handleParameter(int argc, char** argv, string& outerDNS, int& debugLevel, unordered_map<string, string>& table);

//����Ч���ݳ���Ϊsize��buf�л��relay�е�ip��qtype
string getDomainAndQtype(const char* buf, size_t size, unsigned short& qtype);

//���ṩ��ip��buf�й�����Ӧ��,sizeΪԭ������
void buildDatagram(const string& ip, char* buf, size_t& size);

//ʮ�����Ƹ�ʽ����ӡ�ֽ�
void show_bytes(unsigned char* start, size_t len);


#endif




