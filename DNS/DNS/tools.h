#ifndef TOOLS_H
#define TOOLS_H

#include"common.h"

bool handleParameter(int argc, char** argv, string& outerDNS, int& debugLevel, unordered_map<string, string>& table)
{
	ifstream file;
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] == "-d")
			debugLevel = 1;
		else if (argv[i] == "-dd")
			debugLevel = 2;
		else if (isdigit(argv[i][0]))
			outerDNS = argv[i];
		else
		{
			if (file.is_open())
				return false;
			file.open(argv[i]);
			if (!file.is_open())
				return false;
		}
	}
	if (!file.is_open())
		file.open(DEFAULT_FILE);
	string domain, ip;
	while (file >> ip >> domain)
		table[domain] = ip;
	file.close();
	return true;
}

string getDomain(const char* buf, size_t size)
{
	string ans;
	size_t i = HEAD_LEN;
	char curCharLen;
	while (buf[i])
		if (curCharLen = buf[i++])
		{
			while (curCharLen--)
				ans += buf[i++];
			if (buf[i])
				ans += '.';
		}
	return ans;
}

unsigned long ipToUL(const string& s)
{
	unsigned long ans = 0;
	for (char c : s)
		if (c != '.')
		{
			ans *= 10;
			ans += c;
		}
	return ans;
}

void buildPacketFoundInRelay(const string& ip, size_t& size, char* buf)
{
	/*for (int i = 0; i < size; i++)
	{
		if ((buf[i] & 0xf0) == 0x00)
		{
			cout << "0";
		}
		//输出十六进制数
		cout << hex << int((buf[i]) & 0xff) << " ";
	}
	cout << endl;*/
	//设置标志域1000 0001 1000 0000/0101
				   //QR = QueryResponse = 1,表示响应
				   //OPCODE = 0,表示标准查询
				   //AA = AuthoritativeAnswer = 0,表示名字服务器非权限服务器
				   //TC = TrunCated = 0，非截断
				   //RD = RecursionDesired = 1，表示客户端希望得到递归回答
				   //RA = RecursionAvailable = 1，表示可以得到递归响应
				   //Z = Zero = 0，保留字段
				   //RCODE = 0，无差错（本地列表服务）
				   //RCODE = 3，有差错（本地列表屏蔽）
	* (unsigned short*)& buf[2] = htons(0x8180);
	if (ip == BLOCKED)
	{
		*(unsigned short*)& buf[6] = htons(0x0000);
		return;
	}

	*(unsigned short*)& buf[6] = htons(0x0001);
	//name：0xc00c   1100 0000 0000 1100 开头11标志为指针 后为偏移量
	*(unsigned short*)& buf[size] = htons(0xc00c);
	size += 2;
	*(unsigned short*)& buf[size] = htons(0x0001);
	size += 2;
	//class:通常为IN(1)，指Internet数据  1 in
	*(unsigned short*)& buf[size] = htons(0x0001);
	size += 2;
	//ttl:客户程序保留该资源记录的秒数  设为10min 即600s 

	*(unsigned long*)& buf[size] = htonl(0x00000228);
	size += 4;
	//relength:说明资源数据的字节数，对类型1（TYPE A记录）资源数据是4字节的IP地址 
	*(unsigned short*)& buf[size] = htons(0x0004);
	size += 2;
	//rdata:资源数据 ipv4地址
//code4 = ipToUL(ip);
//code4 = boost::asio::ip::address_v4::from_string(ip).to_ulong();
	inet_pton(AF_INET, ip.c_str(), (unsigned long*)& buf[size]);
	size += 4;


	/*for (int i = 0; i < size; i++)
	{
		if ((buf[i] & 0xf0) == 0x00)
		{
			cout << "0";
		}
		//输出十六进制数
		cout << hex << int((buf[i]) & 0xff) << " ";
	}
	cout << endl;*/
}
#endif




