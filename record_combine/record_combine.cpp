#include<iostream>
#include<string>
#include<fstream>
#include<cstring>
#include<cstdlib>
#include<unordered_map>
#include<queue>
#include<vector>
#include<algorithm>
#include<functional>
//#include<execution>
using namespace std;

struct SCmdInfo{
    int64_t localAddr,localPort,remoteAddr,remotePort,localCore;
    uint64_t timestamp;
};
queue<SCmdInfo>qSend,qRecv;
void readRecord(ifstream&ifsRecord)
{
    string cmd;
    SCmdInfo cmdInfo;
    //命令 本地地址  本地端口 远程地址 远程端口 本地核心 时间（纳秒）
    while(ifsRecord>>cmd
        >>cmdInfo.localAddr
        >>cmdInfo.localPort
        >>cmdInfo.remoteAddr
        >>cmdInfo.remotePort
        >>cmdInfo.localCore
        >>cmdInfo.timestamp){
            if(cmd=="send")qSend.push(cmdInfo);
            else if(cmd=="recv")qRecv.push(cmdInfo);
            else cout<<"wrong command "<<cmd<<endl;
    }
}

struct SMsgKeyInfo{
    int64_t srcAddr,srcPort,desAddr,desPort;

    bool operator==(const SMsgKeyInfo&x)
    {
        return srcAddr==x.srcAddr&&srcPort==x.srcPort&&desAddr==x.desAddr&&desPort==x.desPort;
    }
};
struct SHashFunc{
    size_t operator()(const SMsgKeyInfo&msgInfo)const
    {
        return (size_t)msgInfo.srcAddr+msgInfo.srcPort+msgInfo.desAddr+msgInfo.desPort;
    }
};
struct SEqualFunc{
    bool operator()(const SMsgKeyInfo&a,const SMsgKeyInfo&b)const
    {
        return a.srcAddr==b.srcAddr&&a.srcPort==b.srcPort&&a.desAddr==b.desAddr&&a.desPort==b.desPort;
    }
    /* bool operator()(SMsgKeyInfo a,SMsgKeyInfo b)const
    {
        return a.srcAddr==b.srcAddr&&a.srcPort==b.srcPort&&a.desAddr==b.desAddr&&a.desPort==b.desPort;
    } */
};
struct SMsgValueInfo{
    int64_t srcCore;
    uint64_t sendTime;
};
typedef queue<SMsgValueInfo> coreIdQueue_t;
typedef unordered_map<SMsgKeyInfo,coreIdQueue_t,SHashFunc,SEqualFunc> coreIdMap_t;
coreIdMap_t coreIdMap;
struct SMsgPhysicalInfo{
    int64_t srcChiplet,srcCore,desChiplet,desCore;
    uint64_t sentTime,recvTime;
};
vector<SMsgPhysicalInfo>vPhyInfo;
void getCoreTraces()
{
    SMsgKeyInfo msgInfo;
    coreIdMap_t::iterator it;
    while(!qSend.empty()){
        SCmdInfo&top=qSend.front();
        msgInfo={top.localAddr,
            top.localPort,
            top.remoteAddr,
            top.remotePort};
        it=coreIdMap.try_emplace(msgInfo,coreIdQueue_t()).first;
        it->second.push({top.localCore,top.timestamp});
        qSend.pop();
    }
    while(!qRecv.empty()){
        SCmdInfo&top=qRecv.front();
        msgInfo={
            top.remoteAddr,
            top.remotePort,
            top.localAddr,
            top.localPort
        };
        it=coreIdMap.find(msgInfo);
        if(it==coreIdMap.end()||it->second.empty())cout<<"Error: a message not sent\n";
        else{
            vPhyInfo.push_back({
                msgInfo.srcAddr,
                it->second.front().srcCore,
                msgInfo.desAddr,
                top.localCore,
                it->second.front().sendTime,
                top.timestamp
            });
            it->second.pop();
        }
        qRecv.pop();
    }
    coreIdMap.clear();
    /* sort(execution::par_unseq,vPhyInfo.begin(),vPhyInfo.end(),[](const SMsgPhysicalInfo&a,const SMsgPhysicalInfo&b){
        return a.sentTime<b.sentTime;
    }); */
    sort(vPhyInfo.begin(),vPhyInfo.end(),[](const SMsgPhysicalInfo&a,const SMsgPhysicalInfo&b){
        return a.sentTime<b.sentTime;
    });
}

void outputRecord(ofstream&ofsRecords,const vector<SMsgPhysicalInfo>&vRecords)
{
    for(auto&r:vRecords){
        //发送时间（纳秒） 接收时间（纳秒） 源芯粒 源核心 目的芯粒 目的核心
        ofsRecords<<r.sentTime<<' '
            <<r.recvTime<<' '
            <<r.srcChiplet<<' '
            <<r.srcCore<<' '
            <<r.desChiplet<<' '
            <<r.desCore<<'\n';
    }
    ofsRecords.flush();
}

//芯粒数量
int main(int argc,char*argv[])
{
    if(argc!=2){
        cout<<"wrong parameters\n";
        return -1;
    }
    //芯粒数量
    int64_t subnetCnt=atol(argv[1]);
    for(int64_t i=0;i<subnetCnt;++i){
        ifstream ifs("record_"+to_string(i)+".txt");
        readRecord(ifs);
        ifs.close();
    }
    getCoreTraces();
    ofstream ofs("physicalRecords.txt");
    outputRecord(ofs,vPhyInfo);
    vPhyInfo.clear();
    ofs.close();
    return 0;
}
