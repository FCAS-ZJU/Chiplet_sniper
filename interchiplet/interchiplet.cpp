#include <zmq.hpp>
#include<zmq_addon.hpp>
#include <iostream>
#include <string>
#include <numeric>
#include <unordered_map>
#include <queue>
#include <filesystem>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <thread>
#include <sstream>
#include <cstring>
#include <vector>
#include <cstdint>
#include <fstream>
#include <atomic>
#include<deque>

using namespace std;

ofstream logfile("interchiplet.log");

string interchiplet_addr;
uint16_t port_base, subnet_count;

void readConfig()
{
	const char CONFIG_PATH[] = "interchiplet.ini";
	const char INTER_ADDR_ITEM[] = "inter_address";
	const char PORT_BASE_ITEM[] = "port_base";
	const char SUBNET_COUNT[] = "subnet_count";
	if (boost::filesystem::exists(CONFIG_PATH))
	{
		boost::property_tree::ptree root, tag;
		boost::property_tree::ini_parser::read_ini(CONFIG_PATH, root);
		tag = root.get_child("config");
		if (tag.count(INTER_ADDR_ITEM) == 1 && tag.count(PORT_BASE_ITEM) == 1 && tag.count(SUBNET_COUNT) == 1)
		{
			interchiplet_addr = tag.get<string>(INTER_ADDR_ITEM);
			port_base = tag.get<uint16_t>(PORT_BASE_ITEM);
			subnet_count = tag.get<uint16_t>(SUBNET_COUNT);
			logfile << INTER_ADDR_ITEM << ": " << interchiplet_addr << endl
					<< PORT_BASE_ITEM << ": " << port_base << endl
					<< SUBNET_COUNT << ": " << subnet_count << endl;
		}
		else
			logfile << "wrong config file" << endl;
	}
	else
		logfile << "no config file" << endl;
}

const size_t MSG_LEN = 1024;
zmq::context_t *context;
zmq::socket_t **zmqSocketArray;
//vector<deque<zmq::message_t>>zmqBuf;
deque<zmq::message_t>*zmqBuf;
int readAllMsg(zmq::socket_t&skt,size_t sktIdx,zmq::recv_flags flag)
{
	//zmq::socket_t skt=*(zmqSocketArray[sktIdx]);
	auto r=zmq::recv_multipart(skt,back_inserter(zmqBuf[sktIdx]),flag);
	return 0;
}
size_t readMsg(zmq::socket_t&skt,size_t sktIdx, string &str, int flag = ZMQ_DONTWAIT)
{
	//zmq::message_t req;
	char t[MSG_LEN + 1] = {0};
	size_t r;
	//r = skt.recv(t, MSG_LEN, flag);
	if(zmqBuf[sktIdx].empty())readAllMsg(skt,sktIdx,(zmq::recv_flags)flag);
	r=zmqBuf[sktIdx].size();
	if(r>0){
		memcpy(t,zmqBuf[sktIdx].front().data(),zmqBuf[sktIdx].front().size());
		str = t;
		zmqBuf[sktIdx].pop_front();
	}
	/* socket->recv(req);
	str=req.to_string()+'\0'; */
	return r;
}
void writeMsg(zmq::socket_t &skt, const string &str, int flag = ZMQ_DONTWAIT)
{
	char t[MSG_LEN + 1] = {0};
	strncpy(t, str.c_str(), MSG_LEN);
	skt.send(t, str.size() + 1, flag);
}

atomic_bool running(false);
struct SMsg
{
	int64_t desAddr, desPort, srcAddr, srcPort;
	int64_t datum;
};
//typedef unordered_map<int64_t,queue<SMsg>> routerBuf_t;
typedef vector<queue</* SMsg */string>> routerBuf_t;
routerBuf_t routerBuf;
vector<unsigned>reading;
void run()
{
	uint16_t i;
	string str;
	context = new zmq::context_t(1);
	//cout<<"context ok"<<endl;
	//socket=new zmq::socket_t(*context,ZMQ_REP);
	zmqSocketArray = new zmq::socket_t *[subnet_count];
	zmqBuf=new deque<zmq::message_t>[subnet_count];
	for (i = 0; i < subnet_count; ++i)
	{
		routerBuf.push_back(queue<string>());
		reading.push_back(0);
		//zmqBuf.push_back(deque<zmq::message_t>());
		//zmqBuf.emplace_back();
		zmqSocketArray[i] = new zmq::socket_t(*context, zmq::socket_type::pair);
		string addr = interchiplet_addr + ':' + to_string(port_base + i);
		logfile << "bind address " << addr << endl;
		(zmqSocketArray[i])->bind(addr);
		logfile << "open the port of subnet " << i << endl;
	}
	//保证同步开始
	for (i = 0; i < subnet_count; ++i)
	{
		readMsg(*(zmqSocketArray[i]),i, str, 0);
		if (str != "ready")
		{
			logfile << " an error of the subnet " << i << endl;
			return;
		}
		logfile << "subnet " << i << " is ready" << endl;
	}
	for (i = 0; i < subnet_count; ++i)
	{
		writeMsg(*(zmqSocketArray[i]), "start", 0);
		logfile << "start subnet " << i << endl;
	}
	//socket->bind(interchiplet_addr);
	std::cout << "all subnet start\n";
	string cmd;
	//SMsg msg;
	routerBuf_t::iterator it;
	int64_t desAddr;
	size_t r;
	size_t forwardCnt=0;
	size_t getRecvCnt=0;
	size_t getSendCnt=0;
	while (running)
	{
		for (i = 0; i < subnet_count && running; ++i)
		{
			r = readMsg(*(zmqSocketArray[i]),i, str /* ,0 */);
			/* stringstream ss(str);
			ss >> cmd;
			if (cmd == "send")
			{
				ss >> msg.desAddr >> msg.desPort >> msg.srcAddr >> msg.srcPort >> msg.datum;
				it = routerBuf.find(msg.desAddr);
				if (it == routerBuf.end())
				{
					it = routerBuf.emplace(msg.desAddr, queue<SMsg>()).first;
				}
				it->second.push(msg);
				writeMsg("send_ret 0\n");
			}
			else if (cmd == "recv")
			{
				ss >> msg.desAddr;
				it = routerBuf.find(msg.desAddr);
				if (it == routerBuf.end() || it->second.empty())
					writeMsg("recv_ret -1\n");
				else
				{
					writeMsg("recv_ret " + to_string(msg.desAddr) + ' ' +
							 to_string(msg.desPort) + ' ' +
							 to_string(msg.srcAddr) + ' ' +
							 to_string(msg.srcPort) + ' ' +
							 to_string(msg.datum) + '\n');
				}
			} */
			//ss >> msg.desAddr >> msg.desPort >> msg.srcAddr >> msg.srcPort >> msg.datum;
			//logfile<<"recvMsg: "<<r<<endl;
			if (r > 0)
			{
				logfile<<"get "<<r<<" message"<<endl;;
				//desAddr = stol(str);
				stringstream ss(str+'\n');
				ss >> cmd;
				if (cmd == "recv")
				{
					logfile<<"get message: "<<str<<endl;
					if(routerBuf[i].empty()){
						//先挂起，以后再写入
						reading[i]++;
					}else {
						writeMsg(*(zmqSocketArray[i]),routerBuf[i].front()+'\n');
						logfile<<"forward message: "<<routerBuf[i].front()<<endl;
						routerBuf[i].pop();
						forwardCnt++;
					}
					getRecvCnt++;
				}else if(cmd=="send"){
					string d;
					getline(ss,d);
					desAddr=stol(d);
					if(0<=desAddr&&desAddr<subnet_count){
						if(reading[desAddr]==0){
							routerBuf[desAddr].push(d);
							logfile << "get message: " << str << endl;
						}else{
							writeMsg(*(zmqSocketArray[desAddr]),d+'\n');
							logfile<<"forward message: "<<d<<endl;
							forwardCnt++;
							reading[desAddr]--;
						}
					}
					else logfile << "wrong message: " << str << endl;
					getSendCnt++;
				}
				else logfile<<"wrong message: "<<str<<endl;
				/* if (0 <= desAddr && desAddr < subnet_count)
				{
					writeMsg(*(zmqSocketArray[desAddr]), str );
					logfile << "forward message: " << str << endl;
				}
				else
					logfile << "wrong message: " << str << endl; */
			}
		}
		this_thread::sleep_for(1ms);
	}
	//退出语句
	for(i=0;i<subnet_count;++i){
		writeMsg(*(zmqSocketArray[i]),"exit");
		logfile<<"interchiplet exits"<<endl;
	}
	delete[]zmqBuf;
	for (i = 0; i < subnet_count; ++i)
		delete zmqSocketArray[i];
	delete[] zmqSocketArray;
	delete context;
	std::cout<<"Total forward message count: "<<forwardCnt
		<<"\nTotal recv message count: "<<getRecvCnt
		<<"\nTotal send message count: "<<getSendCnt<<endl;
}

int main()
{
	thread *th;
	readConfig();
	running.store(true, memory_order_acq_rel);
	th = new thread(run);
	std::cout << "Press any key to exit\n";
	getchar();
	running.store(false, memory_order_acq_rel);
	th->join();
	delete th;
	logfile.close();
	return 0;
}
