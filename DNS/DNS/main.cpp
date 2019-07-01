#include"common.h"
#include"tools.h"



int main(int argc, char** argv)
{
	string outerDNS(DEFAULT_DNS);
	int debugLevel = 0;
	//由文件读入的域名-ip表
	unordered_map<string, string> table;
	//处理参数
	if (!handleParameter(argc, argv, outerDNS, debugLevel, table))
	{
		//参数出错
		cerr << "Parameter error." << endl;
		return -1;
	}

	WSADATA wsaData;
	//WSA初始化失败
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cerr << "Failed to init socket." << endl;
		return -2;
	}

	SOCKET sock;
	//创建socket失败
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		cerr << "Failed to create socket." << endl;
		return -3;
	}

	//创建本地socket地址
	SOCKADDR_IN local;
	local.sin_family = AF_INET;
	local.sin_port = htons(DEFAULT_PORT);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	//地址与socket绑定失败

	if (bind(sock, (SOCKADDR*)& local, sizeof local))
	{
		cerr << "Failed to bind socket." << endl;
		return -4;
	}

	//服务器地址
	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_port = htons(DEFAULT_PORT);
	server.sin_addr.s_addr = inet_addr(outerDNS.c_str());

	//用户地址
	SOCKADDR_IN user;
	int userLen = sizeof user;

	//缓冲区
	char buf[BUF_SIZE];

	/*
	id池,count为分配的新id,每次count+1,只有当id池满时（65535）才有可能发生冲突,
	采取的策略是直接覆盖,由客户机判断超时,减少服务器压力,对于小规模的客户群完全够用
	*/
	vector<pair<unsigned short, SOCKADDR_IN>> idPool(USHRT_MAX);
	//类型为unsigned short,当达到最大id65535时,再增加自动变为0
	unsigned short count = 0;

	while (true)
	{
		//收包
		size_t size = recvfrom(sock, buf, BUF_SIZE, 0, (SOCKADDR*)& user, &userLen);
		if (size == 0 || size == SOCKET_ERROR)
			continue;

		if (debugLevel > 0)
		{
			cout << "Received, id:";
			unsigned short t = *(unsigned short*)& buf;
			cout << t << endl;
			if (debugLevel == 2)
			{
				cout << "    contents:";
				show_bytes((unsigned char*)buf, size);
				cout << endl;
			}
		}

		bool qr = buf[2] >> 7;
		//查询报
		if (qr == 0)
		{
			if (debugLevel > 0)
				cout << "    Query datagram" << endl;

			unordered_map<string, string>::iterator it;
			unsigned short qtype;
			//获得域名
			string domain = getDomainAndQtype(buf, size, qtype);
			//ipv4且在relay中存在
			if (qtype == 1 && (it = table.find(domain)) != table.end())
			{
				if (debugLevel > 0)
					cout << "    Found in relay. Domain:" << domain << "\tip:" << table[domain] << endl;
				//构建响应包
				buildDatagram(table[domain], buf, size);
				sendto(sock, buf, size, 0, (SOCKADDR*)& user, userLen);
			}
			//向外界服务器查询
			else
			{
				unsigned short oldID = *(unsigned short*)& buf;
				//分配新的id
				unsigned short newID = count++;

				if (debugLevel > 0)
				{
					cout << "    Not found in relay" << endl;
					if (debugLevel == 2)
						cout << "    new id:" << newID << endl;
				}

				auto& it = idPool[newID];
				//转换小端字节序
				it.first = ntohs(oldID);
				//记录用户地址
				it.second = user;

				*(unsigned short*)& buf = htons(newID);
				sendto(sock, buf, size, 0, (SOCKADDR*)& server, sizeof server);
			}
		}
		//响应报
		else if (qr == 1)
		{
			unsigned short newID = *(unsigned short*)& buf;
			//转换小端字节序
			newID = ntohs(newID);
			//获取对应报的信息
			auto& it = idPool[newID];

			if (debugLevel > 0)
			{
				cout << "    Response datagram" << endl;
				if (debugLevel == 2)
					cout << "    new id:" << newID << ",old id:" << it.first << endl;
			}

			//设置为旧id
			*(unsigned short*)& buf = htons(it.first);
			sendto(sock, buf, size, 0, (SOCKADDR*)& it.second, sizeof it.second);
		}

		if (debugLevel == 2)
		{
			cout << "    contents:";
			show_bytes((unsigned char*)buf, size);
			cout << endl;
		}
	}
}