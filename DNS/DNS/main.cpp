#include"common.h"
#include"tools.h"



int main(int argc, char** argv)
{
	string outerDNS(DEFAULT_DNS);
	int debugLevel = 0;
	//���ļ����������-ip��
	unordered_map<string, string> table;
	//�������
	if (!handleParameter(argc, argv, outerDNS, debugLevel, table))
	{
		//��������
		cerr << "Parameter error." << endl;
		return -1;
	}

	WSADATA wsaData;
	//WSA��ʼ��ʧ��
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cerr << "Failed to init socket." << endl;
		return -2;
	}

	SOCKET sock;
	//����socketʧ��
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		cerr << "Failed to create socket." << endl;
		return -3;
	}

	//��������socket��ַ
	SOCKADDR_IN local;
	local.sin_family = AF_INET;
	local.sin_port = htons(DEFAULT_PORT);
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	//��ַ��socket��ʧ��

	if (bind(sock, (SOCKADDR*)& local, sizeof local))
	{
		cerr << "Failed to bind socket." << endl;
		return -4;
	}

	//��������ַ
	SOCKADDR_IN server;
	server.sin_family = AF_INET;
	server.sin_port = htons(DEFAULT_PORT);
	server.sin_addr.s_addr = inet_addr(outerDNS.c_str());

	//�û���ַ
	SOCKADDR_IN user;
	int userLen = sizeof user;

	//������
	char buf[BUF_SIZE];

	/*
	id��,countΪ�������id,ÿ��count+1,ֻ�е�id����ʱ��65535�����п��ܷ�����ͻ,
	��ȡ�Ĳ�����ֱ�Ӹ���,�ɿͻ����жϳ�ʱ,���ٷ�����ѹ��,����С��ģ�Ŀͻ�Ⱥ��ȫ����
	*/
	vector<pair<unsigned short, SOCKADDR_IN>> idPool(USHRT_MAX);
	//����Ϊunsigned short,���ﵽ���id65535ʱ,�������Զ���Ϊ0
	unsigned short count = 0;

	while (true)
	{
		//�հ�
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
		//��ѯ��
		if (qr == 0)
		{
			if (debugLevel > 0)
				cout << "    Query datagram" << endl;

			unordered_map<string, string>::iterator it;
			unsigned short qtype;
			//�������
			string domain = getDomainAndQtype(buf, size, qtype);
			//ipv4����relay�д���
			if (qtype == 1 && (it = table.find(domain)) != table.end())
			{
				if (debugLevel > 0)
					cout << "    Found in relay. Domain:" << domain << "\tip:" << table[domain] << endl;
				//������Ӧ��
				buildDatagram(table[domain], buf, size);
				sendto(sock, buf, size, 0, (SOCKADDR*)& user, userLen);
			}
			//������������ѯ
			else
			{
				unsigned short oldID = *(unsigned short*)& buf;
				//�����µ�id
				unsigned short newID = count++;

				if (debugLevel > 0)
				{
					cout << "    Not found in relay" << endl;
					if (debugLevel == 2)
						cout << "    new id:" << newID << endl;
				}

				auto& it = idPool[newID];
				//ת��С���ֽ���
				it.first = ntohs(oldID);
				//��¼�û���ַ
				it.second = user;

				*(unsigned short*)& buf = htons(newID);
				sendto(sock, buf, size, 0, (SOCKADDR*)& server, sizeof server);
			}
		}
		//��Ӧ��
		else if (qr == 1)
		{
			unsigned short newID = *(unsigned short*)& buf;
			//ת��С���ֽ���
			newID = ntohs(newID);
			//��ȡ��Ӧ������Ϣ
			auto& it = idPool[newID];

			if (debugLevel > 0)
			{
				cout << "    Response datagram" << endl;
				if (debugLevel == 2)
					cout << "    new id:" << newID << ",old id:" << it.first << endl;
			}

			//����Ϊ��id
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