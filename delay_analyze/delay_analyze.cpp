#include<iostream>
#include<fstream>
#include<cstdint>
#include<queue>
#include<unordered_map>
#include<vector>
#include<algorithm>
using namespace std;

struct SSession{
    int64_t srcChiplet,srcCore,desChiplet,desCore;
};
struct SMsgPhyInfo{
    double sentTime,recvTime;
    int64_t srcChiplet,srcCore,desChiplet,desCore;
};
struct SHashFunc{
    size_t operator()(const SSession&s)const
    {
        return s.srcChiplet+s.srcCore+s.desChiplet+s.desCore;
    }
};
struct SEqualFunc{
    bool operator()(const SSession&a,const SSession&b)const
    {
        return a.srcChiplet==b.srcChiplet&&a.srcCore==b.srcCore&&a.desChiplet==b.desChiplet&&a.desCore==b.desCore;
    }
};
unordered_map<SSession,queue<SMsgPhyInfo>,SHashFunc,SEqualFunc>msgInfoMap;

void readMsgFile(ifstream&ifsMsgFile)
{
    SMsgPhyInfo msgPhyInfo;
    //发送时间（纳秒） 接收时间（纳秒） 源芯粒 源核心 目的芯粒 目的核心
    while(ifsMsgFile>>msgPhyInfo.sentTime
            >>msgPhyInfo.recvTime
            >>msgPhyInfo.srcChiplet>>msgPhyInfo.srcCore
            >>msgPhyInfo.desChiplet>>msgPhyInfo.desCore){
        msgInfoMap.try_emplace({
            msgPhyInfo.srcChiplet,
            msgPhyInfo.srcCore,
            msgPhyInfo.desChiplet,
            msgPhyInfo.desCore
        }).first->second.push(msgPhyInfo);
    }
}
int64_t chipletCnt,coreCnt;
double**totalDelay;
void readDelayFile(ifstream&ifsDelayFile)
{
    double sentTime,delay;
    SSession session;
    //发送时间（ns），源地址，目的地址，延迟（ns）
    while(ifsDelayFile>>sentTime
            >>session.srcChiplet>>session.srcCore
            >>session.desChiplet>>session.desCore
            >>delay){
        auto it=msgInfoMap.find(session);
        if(it==msgInfoMap.end()||it->second.empty())cout<<"Error: a message not found"<<endl;
        else{
            totalDelay[session.desChiplet][session.desCore]+=max(0.0,sentTime+delay-it->second.front().recvTime);
            it->second.pop();
        }
    }
}

//芯粒数量，每块芯粒的核心数量
int main(int argc,char**argv)
{
    if(argc!=3){
        cout<<"Wrong number of parameters\n";
        return 1;
    }
    chipletCnt=atol(argv[1]);
    coreCnt=atol(argv[2]);
    int64_t i;
    totalDelay=new double*[chipletCnt];
    for(i=0;i<chipletCnt;++i){
        totalDelay[i]=new double[coreCnt];
        fill_n(totalDelay[i],coreCnt,0.0);
    }
    ifstream ifsMsg("physicalRecords.txt");
    readMsgFile(ifsMsg);
    ifsMsg.close();
    ifstream ifsDelay("delayPhyInfo.txt");
    readDelayFile(ifsDelay);
    ifsDelay.close();
    cout<<"Total delay:\n";
    for(i=0;i<chipletCnt;++i){
        for(int64_t j=0;j<coreCnt;++j)
            cout<<"Chiplet "<<i<<" Core "<<j<<" total delay: "<<totalDelay[i][j]<<endl;
    }
    for(i=0;i<chipletCnt;++i)delete[]totalDelay[i];
    delete[]totalDelay;
    return 0;
}
