#include"common.h"
#include"tools.h"
#include<queue>


void handleIDTList(deque<IDTransferNode>& idtlist, int& idtlist_base)
{
	//如果队列已满
	if (idtlist.size() == IDT_MAX_SIZE)
	{
		//丢弃前半部分的请求
		for (int i = 0; i < IDT_HALF_SIZE; i++)
		{
			idtlist.pop_front();
		}
		//调整队列的起始下标为500
		if (idtlist_base == 0)
		{
			idtlist_base = IDT_HALF_SIZE;
		}
		else
		{
			idtlist_base = 0;
		}
	}
}

int pos2id(deque<IDTransferNode> idtlist,
	int idtlist_base)
{
	return ((idtlist.size() + idtlist_base) % IDT_MAX_SIZE);
}

int id2pos(int x, int idtlist_base)
{
	return((x + idtlist_base) % IDT_MAX_SIZE);
}

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
	memset(inbuf, 0, BUF_SIZE);
	memset(outbuf, 0, BUF_SIZE);
	deque<IDTransferNode> idtList;
	//记录一个请求的节点
	IDTransferNode idtNode;
	//初始时队列基准为0，然后超过1000时变为500
	int idtListBase = 0;

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
				buildPacketFoundInRelay(table[domain], size, inbuf);
				memcpy(outbuf, inbuf, size);
				sock.send_to(buffer(outbuf), sender_ep);
			}
			else
			{
				unsigned short* oldID = (unsigned short*)malloc(sizeof(unsigned short));
				memcpy(oldID, inbuf, 2);
				//保证队列不为满
				handleIDTList(idtList, idtListBase);

				//为确保中继DNS的id具有唯一性
				int newIDint = pos2id(idtList, idtListBase);
				unsigned short newID = htons((unsigned short)(newIDint));

				//变更中继DNS包的id，使之newID唯一,并记录oldID,cname
				idtNode.oldID = ntohs(*oldID);
				idtNode.ep = sender_ep;
				idtNode.processed = false;
				idtList.push_back(idtNode);

				memcpy(inbuf, &newID, 2);

				sock.send_to(buffer(inbuf), ep);
				free(oldID);
				/*	string send(inbuf, size);
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
					sock.send_to(buffer(inbuf), ep);*/
			}
		}
		else
		{
			unsigned short* newID = (unsigned short*)malloc(sizeof(unsigned short));

			//从buffer获取新ID
			memcpy(newID, inbuf, 2);

			//将新ID转成int型，ntohs = net to host short，网络顺序转成主机顺序
			int a = (int)ntohs(*newID);
			free(newID);

			//获取新ID对应的旧ID的下标
			int pos = id2pos(a, idtListBase);

			//若当该query已处理，则直接跳过
			if (idtList[pos].processed) continue;

			//若当该query未处理，获取旧ID，htons = host to net short，主机顺序转成网络顺序
			unsigned short oldID = htons(idtList[pos].oldID);

			//构造响应报文头ID，准备发送回客户端
			memcpy(inbuf, &oldID, 2);

			//标记为已处理
			idtList[pos].processed = true;
			sock.send_to(buffer(inbuf), idtList[pos].ep);
			/*unsigned short newId;
			memcpy(&newId, inbuf, 2);
			auto& it = id[newId];
			if (!get<2>(it))
				continue;
			memcpy(inbuf, &get<0>(it), 2);
			sock.send_to(buffer(inbuf), get<1>(it));
			get<2>(it) = false;
			unused.push(newId);*/
		}
	}
}