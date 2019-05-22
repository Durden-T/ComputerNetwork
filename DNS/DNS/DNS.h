#ifndef DNS_H
#define DNS_H

#include"common.h"
#include<boost/asio.hpp>

const string defaultDNS("202.106.0.20");
const string defaultFile("dnsrelay.txt");

using namespace boost::asio;

//struct 
class DNS
{
public:
	DNS(const string& _outerDNS, const string& fileName, int _debugLevel) : outerDNS(_outerDNS.empty() ? defaultDNS : _outerDNS), debugLevel(_debugLevel)
	{
		file.open(fileName.empty() ? defaultFile : fileName);
		ep = { ip::address::from_string(outerDNS), 53 };
	}

	~DNS();

	void run();

	void query(const string& request);
private:


	io_service service;
	ip::udp::socket sock = ip::udp::socket(service, ip::udp::endpoint(ip::udp::v4(), PORT));
	ip::udp::endpoint sender_ep;
	ip::udp::endpoint ep;

	string outerDNS;
	fstream file;
	unordered_map<string, string> table;
	int debugLevel;

	char buff[1024];
};

#endif