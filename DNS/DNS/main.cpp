#include"common.h"
#include"tools.h"
#include<queue>



int main(int argc, char** argv)
{
	string outerDNS = DEFAULT_DNS;
	int debugLevel = 0;
	unordered_map<string, string> table;

	if (!handleParameter(argc, argv, outerDNS, debugLevel, table))
	{
		cerr << "命令行参数错误。" << endl;
		return -1;
	}

	io_service service;
	ip::udp::socket sock(service, ip::udp::endpoint(ip::udp::v4(), PORT));
	ip::udp::endpoint ep(ip::address_v4::from_string(outerDNS), PORT);
	ip::udp::endpoint sender_ep;
	char inbuf[BUF_SIZE], outbuf[BUF_SIZE];
	/*queue<unsigned short> unused;
	for (unsigned short i = 0; i <= USHRT_MAX; ++i)
		unused.push(i);
	//unordered_map<unsigned short, pair<unsigned short, ip::udp::endpoint> > id;
	unordered_map<unsigned short, tuple<unsigned short, ip::udp::endpoint,bool> > id;*/

	while (true)
	{
		size_t size = sock.receive_from(buffer(inbuf), sender_ep);
		bool qr = inbuf[2] >> 7;
		if (qr == 0)//查询报
		{
			unordered_map<string, string>::iterator it;
			string domain = getDomain(inbuf, size);
			if ((it = table.find(domain)) != table.end())//在relay中找到
			{
				buildPacketFoundInRelay(domain, size, inbuf, outbuf);
				sock.send_to(buffer(outbuf), sender_ep);
			}
			else
			{
				string send(inbuf, size);
				sock.send_to(buffer(send), ep);
				int newsize = sock.receive_from(buffer(inbuf), ep);
				send = string(inbuf, newsize);
				sock.send_to(buffer(send), sender_ep);
				if (unused.empty())
					continue;
				unsigned short oldId;
				memcpy(&oldId, inbuf, 2);
				unsigned short newId = unused.front();
				unused.pop();
				id[newId] = make_tuple(oldId, sender_ep,true);
				memcpy(inbuf, &newId, 2);
				sock.send_to(buffer(inbuf), ep);
			}
		}
		/*else
		{
			unsigned short newId;
			memcpy(&newId, inbuf, 2);
			auto& it = id[newId];
			if (!get<2>(it))
				continue;
			memcpy(inbuf, &get<0>(it), 2);
			sock.send_to(buffer(inbuf), get<1>(it));
			get<2>(it) = false;
			unused.push(newId);
		}*/
	}
}