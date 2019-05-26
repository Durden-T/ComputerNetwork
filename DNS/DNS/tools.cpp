#include"common.h"
#include"tools.h"



bool handleParameter(int argc, char** argv, string& outerDNS, int& debugLevel, unordered_map<string, string>& table)
{
	ifstream file;
	for (int i = 1; i < argc; ++i)
	{
		//����Ϊ1
		if (!strcmp(argv[i], "-d"))
			debugLevel = 1;
		//����Ϊ2
		else if (!strcmp(argv[i], "-dd"))
			debugLevel = 2;
		//�ṩdns������
		else if (isdigit(argv[i][0]))
			outerDNS = argv[i];
		//�ж��Ƿ�Ϊ�ļ�
		else
		{
			//�����ļ���ַ
			if (file.is_open())
				return false;
			file.open(argv[i]);
			//�����ļ���ַ,��������
			if (!file.is_open())
				return false;
			if (debugLevel > 0)
				cout << "File: " << argv[i];
		}
	}
	//�ļ�δ�򿪹�,����ΪĬ���ļ���ַ
	if (!file.is_open())
	{
		file.open(DEFAULT_FILE);
		if (debugLevel > 0)
			cout << "File: " << DEFAULT_FILE;
	}

	string domain, ip;
	//ѭ����,����table��
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
	//������ʽΪ3www6google3com0
	string ans;
	//����header����
	size_t i = HEAD_LEN;
	//ͳ�Ƴ���
	char curCharLen;
	//��buf[i]��Ϊ'\0',����������δ����ʱѭ��
	while (buf[i])
	{
		//����������curCharLen,++i
		if (curCharLen = buf[i++])
		{
			//ans+curCharLen���ַ�
			while (curCharLen--)
				ans += buf[i++];
			//δ���������ֵĽ�β,���'.'
			if (buf[i])
				ans += '.';
		}
	}
	qtype = ntohs(*(unsigned short*)& buf[++i]);
	return ans;
}

void buildDatagram(const string& ip, char* buf, size_t& size)
{
	//header��(ͨ��ץ�����Ķ�Э��õ�)
	//qr=1,opcode=0,aa=0,tc=0,rd=1,ra=1,z=0,rcode=0/3(������Ӧ/��������)

	//ipΪ������״̬
	if (ip == BLOCKED)
	{
		//����header
		*(unsigned short*)& buf[2] = htons(0x8183);
		//����ancountΪ0
		*(unsigned short*)& buf[6] = htons(0x0000);
		//ֱ�ӷ���
		return;
	}

	//����header
	*(unsigned short*)& buf[2] = htons(0x8180);
	//����ancountΪ1
	*(unsigned short*)& buf[6] = htons(0x0001);

	//name��0xc00c,��ͷ11��־Ϊָ��,��Ϊƫ����
	*(unsigned short*)& buf[size] = htons(0xc00c);
	size += 2;

	//type:1 
	*(unsigned short*)& buf[size] = htons(0x0001);
	size += 2;

	//class:1,in
	*(unsigned short*)& buf[size] = htons(0x0001);
	size += 2;

	//ttl:��Ϊ10min,��600s 
	*(unsigned long*)& buf[size] = htonl(0x00000228);
	size += 4;

	//relength:˵����Դ���ݵ��ֽ�����������1��TYPE A��¼����Դ������4�ֽڵ�IP��ַ 
	*(unsigned short*)& buf[size] = htons(0x0004);
	size += 2;

	//rdata:��Դ���� ipv4��ַ
	*(unsigned long*)& buf[size] = inet_addr(ip.c_str());
	size += 4;
}

void show_bytes(unsigned char* start, size_t len)
{
	for (int i = 0; i < len; i++)
		printf(" %.2x", start[i]);
}
