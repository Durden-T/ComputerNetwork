#ifndef COMMON_H
#define COMMON_H



#pragma comment(lib,"Ws2_32.lib")
#include <winsock2.h>
#include <windows.h>
#include<iostream>
#include<fstream>
#include<unordered_map>

using namespace std;

const unsigned short DEFAULT_PORT(53);
const string DEFAULT_DNS("10.3.9.4");
const string DEFAULT_FILE("dnsrelay.txt");
const size_t BUF_SIZE(512);
const size_t HEAD_LEN(12);
const string BLOCKED("0.0.0.0");



#endif