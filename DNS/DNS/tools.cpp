#include"common.h"
#include"tools.h"



bool handleParameter(int argc, char** argv, string& outerDNS, int& debugLevel, unordered_map<string, string>& table)
{
	ifstream file;
	for (int i = 1; i < argc; ++i)
	{
		//级别为1
		if (!strcmp(argv[i], "-d"))
			debugLevel = 1;
		//级别为2
		else if (!strcmp(argv[i], "-dd"))
			debugLevel = 2;
		//提供dns服务器
		else if (isdigit(argv[i][0]))
			outerDNS = argv[i];
		//判断是否为文件
		else
		{
			//已有文件地址
			if (file.is_open())
				return false;
			file.open(argv[i]);
			//不是文件地址,参数错误
			if (!file.is_open())
				return false;
			if (debugLevel > 0)
				cout << "File: " << argv[i];
		}
	}
	//文件未打开过,设置为默认文件地址
	if (!file.is_open())
	{
		file.open(DEFAULT_FILE);
		if (debugLevel > 0)
			cout << "File: " << DEFAULT_FILE;
	}

	string domain, ip;
	//循环读,放入table中
	while (file >> ip >> domain)
		table[domain] = ip;

	file.close();

	if (debugLevel > 0)
	{
		cout << ", outerDNS: " << outerDNS;
		if (debugLevel == 2)
		{
			cout << ", relay contents: " << endl;
			for (auto& i : table)
				cout << i.first << '\t' << i.second << endl;
		}
	}

	return true;
}

string getDomainAndQtype(const char* buf, size_t size, unsigned short& qtype)
{
	//域名格式为3www6google3com0
	string ans;
	//跳过header部分
	size_t i = HEAD_LEN;
	//统计长度
	char curCharLen;
	//当buf[i]不为'\0',即域名部分未结束时循环
	while (buf[i])
	{
		//将数量赋给curCharLen,++i
		if (curCharLen = buf[i++])
		{
			//ans+curCharLen个字符
			while (curCharLen--)
				ans += buf[i++];
			//未到域名部分的结尾,添加'.'
			if (buf[i])
				ans += '.';
		}
	}
	qtype = ntohs(*(unsigned short*)& buf[++i]);
	return ans;
}

void buildDatagram(const string& ip, char* buf, size_t& size)
{
	//header：(通过抓包和阅读协议得到)
	//qr=1,opcode=0,aa=0,tc=0,rd=1,ra=1,z=0,rcode=0/3(正常响应/发生错误)

	//ip为被屏蔽状态
	if (ip == BLOCKED)
	{
		//设置header
		*(unsigned short*)& buf[2] = htons(0x8183);
		//设置ancount为0
		*(unsigned short*)& buf[6] = htons(0x0000);
		//直接返回
		return;
	}

	//设置header
	*(unsigned short*)& buf[2] = htons(0x8180);
	//设置ancount为1
	*(unsigned short*)& buf[6] = htons(0x0001);

	//name：0xc00c,开头11标志为指针,后为偏移量
	*(unsigned short*)& buf[size] = htons(0xc00c);
	size += 2;

	//type:1 
	*(unsigned short*)& buf[size] = htons(0x0001);
	size += 2;

	//class:1,in
	*(unsigned short*)& buf[size] = htons(0x0001);
	size += 2;

	//ttl:设为10min,即600s 
	*(unsigned long*)& buf[size] = htonl(0x00000228);
	size += 4;

	//relength:说明资源数据的字节数，对类型1（TYPE A记录）资源数据是4字节的IP地址 
	*(unsigned short*)& buf[size] = htons(0x0004);
	size += 2;

	//rdata:资源数据 ipv4地址
	*(unsigned long*)& buf[size] = inet_addr(ip.c_str());
	size += 4;
}

void show_bytes(unsigned char* start, size_t len)
{
	for (int i = 0; i < len; i++)
		printf(" %.2x", start[i]);
}
