#pragma comment(lib,"Ws2_32.lib")


#include <winsock2.h>
#include <windows.h>
#include"common.h"
#include"tools.h"



struct DatagramInfo
{
	unsigned short oldID;
	SOCKADDR_IN user;
	bool valid;
};



int main(int argc, char** argv)
{
	string outerDNS = DEFAULT_DNS;
	int debugLevel = 0;
	//由文件读入的域名-ip表
	unordered_map<string, string> table;
	if (!handleParameter(argc, argv, outerDNS, debugLevel, table))
	{
		cerr << "Parameter error." << endl;
		return -1;
	}



	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cerr << "Failed to init socket." << endl;
		return -2;
	}
	SOCKET sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		cerr << "Failed to create socket." << endl;
		return -3;
	}

	SOCKADDR_IN local;
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	local.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock, (SOCKADDR*)& local, sizeof(local)))
	{
		cerr << "Failed to bind socket." << endl;
		return -4;
	}

	//服务器地址
	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = inet_addr(outerDNS.c_str());

	//用户地址
	SOCKADDR_IN user;
	int userLen = sizeof(user);

	char buf[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	//临时包
	DatagramInfo tmp;

	//可用ID
	queue<unsigned short> unused;
	for (int i = 0; i <= USHRT_MAX; ++i)
		unused.push(i);
	//新ID与包的信息的hash表
	unordered_map<unsigned short, DatagramInfo > idTable;

	while (true)
	{
		//收包
		size_t size = recvfrom(sock, buf, BUF_SIZE, 0, (SOCKADDR*)& user, &userLen);
		if (size == 0 || size == SOCKET_ERROR)
		{
			cerr << "Receiving error." << endl;
			continue;
		}

		if (debugLevel > 0)
		{
			cout << "Received, id:";
			unsigned short t = *(unsigned short*)& buf;
			cout << t << endl;
			if (debugLevel == 2)
			{
				cout << "    contents:\n   ";
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
			//获得域名
			string domain = getDomain(buf, size);
			//在relay中存在
			if ((it = table.find(domain)) != table.end())
			{
				if (debugLevel > 0)
					cout << "    Found in relay. Domain:" << domain << "\tip:" << table[domain] << endl;
				//构建响应包
				buildDatagramFoundInRelay(table[domain], size, buf);
				sendto(sock, buf, size, 0, (SOCKADDR*)& user, userLen);
			}
			//向外界服务器查询
			else
			{
				if (unused.empty())
				{
					cerr << "No available ID." << endl;
					continue;
				}
				unsigned short oldID = *(unsigned short*)& buf;
				//从unused中获得一个新ID
				unsigned short newID = unused.front();
				//移除此ID
				unused.pop();

				if (debugLevel > 0)
				{
					cout << "    Not found in relay" << endl;
					if (debugLevel == 2)
						cout << "    " << unused.size() << " ids are unused, new id:" << newID << endl;
				}

				//转换成unsigned short
				tmp.oldID = ntohs(oldID);
				//记录用户地址
				tmp.user = user;
				//lasy erase
				tmp.valid = true;
				//插入到hash表中
				idTable.emplace(newID, tmp);
				//将id设为新id
				newID = htons(newID);
				*(unsigned short*)& buf = newID;
				sendto(sock, buf, size, 0, (SOCKADDR*)& server, sizeof(server));
			}
		}
		//响应报
		else if (qr == 1)
		{

			unsigned short newID = *(unsigned short*)& buf;
			newID = ntohs(newID);
			//获取对应包的信息
			auto& it = idTable[newID];
			//无效跳过
			if (!it.valid)
				continue;

			if (debugLevel > 0)
			{
				cout << "    Response datagram" << endl;
				if (debugLevel == 2)
					cout << "    new id:" << newID << ",old id:" << it.oldID << endl;
			}

			//设置为旧id
			*(unsigned short*)& buf = htons(it.oldID);
			sendto(sock, buf, size, 0, (SOCKADDR*)& it.user, sizeof(it.user));

			//设为可用状态
			it.valid = true;
			//放回unused
			unused.push(newID);
		}

		if (debugLevel == 2)
		{
			cout << "    contents:\n   ";
			show_bytes((unsigned char*)buf, size);
			cout << endl;
		}
	}
}