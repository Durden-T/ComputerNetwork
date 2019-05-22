#include "DNS.h"

DNS::~DNS()
{
	file.close();
}

void DNS::run()
{
	int bytes = sock.receive_from(buffer(buff), sender_ep);

	string rec(buff, bytes);

	query(rec);

	sock.send_to(buffer(buff), sender_ep);
}

void DNS::query(const string& request)
{
	if(table.count())
}
