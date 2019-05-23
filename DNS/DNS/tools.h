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

void buildPacketFoundInRelay(const string& domain, size_t size, char* inbuf, char* outbuf)
{
	memcpy(outbuf, inbuf, size);
	unsigned short code2 = inbuf[2];
	//设置标志域1000 0001 1000 0000/0101
				   //QR = QueryResponse = 1,表示响应
				   //OPCODE = 0,表示标准查询
				   //AA = AuthoritativeAnswer = 0,表示名字服务器非权限服务器
				   //TC = TrunCated = 0，非截断
				   //RD = RecursionDesired = 1，表示客户端希望得到递归回答
				   //RA = RecursionAvailable = 1，表示可以得到递归响应
				   //Z = Zero = 0，保留字段
				   //RCODE = 0，无差错（本地列表服务）
				   //RCODE = 5，有差错（本地列表屏蔽）
	if (domain == BLOCKED)
	{
		code2 = htons(0x8185);
	}
	else
	{
		code2 = htons(0x8180);
	}
	memcpy(&outbuf[2], &code2, 2);

	if (domain == BLOCKED)
	{
		code2 = htons(0x0000);
	}
	else
	{
		code2 = htons(0x0001);
	}
	memcpy(&outbuf[6], &code2, 2);

	int RRStart = size;

	//name：0xc00c   1100 0000 0000 1100 开头11标志为指针 后为偏移量
	code2 = htons(0xc00c);
	memcpy(&outbuf[RRStart], &code2, 2);
	RRStart += 2;

	//type:RR的类型码 1 IPV4
	code2 = htons(0x0001);
	memcpy(&outbuf[RRStart], &code2, 2);
	RRStart += 2;

	//class:通常为IN(1)，指Internet数据  1 in
	code2 = htons(0x0001);
	memcpy(&outbuf[RRStart], &code2, 2);
	RRStart += 2;
	//ttl:客户程序保留该资源记录的秒数  设为10min 即600s 
	unsigned long code4;
	code4 = htonl(0x00000258);
	memcpy(&outbuf[RRStart], &code4, 4);
	RRStart += 4;

	//relength:说明资源数据的字节数，对类型1（TYPE A记录）资源数据是4字节的IP地址 
	code2 = htons(0x0004);
	memcpy(&outbuf[RRStart], &code2, 2);
	RRStart += 2;
	//rdata:资源数据 ipv4地址
	//code4 = ipToUL(domain);
	//code4 = boost::asio::ip::address_v4::from_string(domain).to_ulong();
	code4 = inet_addr(domain.c_str());
	memcpy(&outbuf[RRStart], &code4, 4);
	RRStart += 4;
}
#endif
