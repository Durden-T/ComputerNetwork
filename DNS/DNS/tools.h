#ifndef TOOLS_H
#define TOOLS_H



#include"common.h"


//解析处理参数,发生参数错误返回false,否则返回true
bool handleParameter(int argc, char** argv, string& outerDNS, int& debugLevel, unordered_map<string, string>& table);

//从有效数据长度为size的buf中获得relay中的ip
string getDomain(const char* buf, size_t size);

//用提供的ip在buf中构建响应包,size为原包长度
void buildDatagramFoundInRelay(const string& ip, size_t& size, char* buf);

//十六进制打印字节
void show_bytes(unsigned char* start, size_t len);



#endif




