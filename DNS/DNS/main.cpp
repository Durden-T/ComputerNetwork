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
	//���ļ����������-ip��
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

	//��������ַ
	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = inet_addr(outerDNS.c_str());

	//�û���ַ
	SOCKADDR_IN user;
	int userLen = sizeof(user);

	char buf[BUF_SIZE];
	memset(buf, 0, BUF_SIZE);
	//��ʱ��
	DatagramInfo tmp;

	//����ID
	queue<unsigned short> unused;
	for (int i = 0; i <= USHRT_MAX; ++i)
		unused.push(i);
	//��ID�������Ϣ��hash��
	unordered_map<unsigned short, DatagramInfo > idTable;

	while (true)
	{
		//�հ�
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
		//��ѯ��
		if (qr == 0)
		{
			if (debugLevel > 0)
				cout << "    Query datagram" << endl;

			unordered_map<string, string>::iterator it;
			//�������
			string domain = getDomain(buf, size);
			//��relay�д���
			if ((it = table.find(domain)) != table.end())
			{
				if (debugLevel > 0)
					cout << "    Found in relay. Domain:" << domain << "\tip:" << table[domain] << endl;
				//������Ӧ��
				buildDatagramFoundInRelay(table[domain], size, buf);
				sendto(sock, buf, size, 0, (SOCKADDR*)& user, userLen);
			}
			//������������ѯ
			else
			{
				if (unused.empty())
				{
					cerr << "No available ID." << endl;
					continue;
				}
				unsigned short oldID = *(unsigned short*)& buf;
				//��unused�л��һ����ID
				unsigned short newID = unused.front();
				//�Ƴ���ID
				unused.pop();

				if (debugLevel > 0)
				{
					cout << "    Not found in relay" << endl;
					if (debugLevel == 2)
						cout << "    " << unused.size() << " ids are unused, new id:" << newID << endl;
				}

				//ת����unsigned short
				tmp.oldID = ntohs(oldID);
				//��¼�û���ַ
				tmp.user = user;
				//lasy erase
				tmp.valid = true;
				//���뵽hash����
				idTable.emplace(newID, tmp);
				//��id��Ϊ��id
				newID = htons(newID);
				*(unsigned short*)& buf = newID;
				sendto(sock, buf, size, 0, (SOCKADDR*)& server, sizeof(server));
			}
		}
		//��Ӧ��
		else if (qr == 1)
		{

			unsigned short newID = *(unsigned short*)& buf;
			newID = ntohs(newID);
			//��ȡ��Ӧ������Ϣ
			auto& it = idTable[newID];
			//��Ч����
			if (!it.valid)
				continue;

			if (debugLevel > 0)
			{
				cout << "    Response datagram" << endl;
				if (debugLevel == 2)
					cout << "    new id:" << newID << ",old id:" << it.oldID << endl;
			}

			//����Ϊ��id
			*(unsigned short*)& buf = htons(it.oldID);
			sendto(sock, buf, size, 0, (SOCKADDR*)& it.user, sizeof(it.user));

			//��Ϊ����״̬
			it.valid = true;
			//�Ż�unused
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