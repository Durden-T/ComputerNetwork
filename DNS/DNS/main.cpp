#include"common.h"
#include"DNS.h"

int main(int argc, char** argv)
{
	const char* outerDNS = nullptr, * fileName = nullptr;
	int debugLevel = 0;
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i] == "-d")
		{
			debugLevel = 1;
		}
		else if (argv[i] == "-dd")
		{
			debugLevel = 2;
		}
		else if (argv[i][0] >= '0' && argv[i][0] <= '9')
			outerDNS = argv[i];
		else
		{
			fstream tmp(fileName);
			if (tmp)
			{
				fileName = argv[i];
				tmp.close();
			}
			else
			{
				cerr << "ÃüÁîÐÐ²ÎÊý´íÎó¡£" << endl;
				return -1;
			}
		}
	}

	DNS dns(outerDNS, fileName, debugLevel);
}