#ifndef TOOLS_H
#define TOOLS_H



#include"common.h"


//�����������,�����������󷵻�false,���򷵻�true
bool handleParameter(int argc, char** argv, string& outerDNS, int& debugLevel, unordered_map<string, string>& table);

//����Ч���ݳ���Ϊsize��buf�л��relay�е�ip
string getDomain(const char* buf, size_t size);

//���ṩ��ip��buf�й�����Ӧ��,sizeΪԭ������
void buildDatagramFoundInRelay(const string& ip, size_t& size, char* buf);

//ʮ�����ƴ�ӡ�ֽ�
void show_bytes(unsigned char* start, size_t len);



#endif




