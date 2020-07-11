#include<iostream>
#include<fstream>
#include<cstdint>
#include<queue>
#include<cstdlib>
#include<vector>
using namespace std;

int64_t chipletPerLineCnt,corePerLineCnt;
double frequency;

struct SDelayInfo{
    double sentTime/* ,recvTime */;
    int64_t srcChiplet,srcCore,desChiplet,desCore;
    double delay;
};
auto cmp=[](const SDelayInfo&a,const SDelayInfo&b){
    return a.sentTime>b.sentTime;
};
priority_queue<SDelayInfo,vector<SDelayInfo>,decltype(cmp)>pq(cmp);
void readFile(ifstream&ifsDelayInfo)
{
    const double TIME_UNIT_FACTOR=1;
    double sentTime/* ,recvTime */;
    int64_t srcChipletX,srcChipletY,srcCoreX,srcCoreY;
    int64_t desChipletX,desChipletY,desCoreX,desCoreY;
    double delay;
    //发送时间（周期），源地址，目的地址，延迟（周期）
    while(ifsDelayInfo>>sentTime
            //>>recvTime
            >>srcChipletX>>srcChipletY>>srcCoreX>>srcCoreY
            >>desChipletX>>desChipletY>>desCoreX>>desCoreY
            >>delay){
        pq.push({
            sentTime/TIME_UNIT_FACTOR/frequency,
            srcChipletX+chipletPerLineCnt*srcChipletY,
            srcCoreX+corePerLineCnt*srcCoreY,
            desChipletX+chipletPerLineCnt*desChipletY,
            desCoreX+corePerLineCnt*desCoreY,
            delay/TIME_UNIT_FACTOR/frequency
        });
    }
}

void writeFile(ofstream&ofsPhyDelayInfo)
{
    while(!pq.empty()){
        const SDelayInfo&top=pq.top();
        //发送时间（ns），源地址，目的地址，延迟（ns）
        ofsPhyDelayInfo<<top.sentTime<<' '
            <<top.srcChiplet<<' '<<top.srcCore<<' '
            <<top.desChiplet<<' '<<top.desCore<<' '
            <<top.delay<<'\n';
        pq.pop();
    }
    ofsPhyDelayInfo.flush();
}

//芯粒每行数量，芯粒内核心每行数量，频率（GHz）
int main(int argc,char**argv)
{
    if(argc!=4){
        cout<<"Wrong number of parameter\n";
        return 1;
    }
    chipletPerLineCnt=atol(argv[1]);
    corePerLineCnt=atol(argv[2]);
    frequency=atof(argv[3]);
    cout<<"Reading...\n";
    ifstream ifs("delayInfo.txt");
    readFile(ifs);
    ifs.close();
    cout<<"Writing...\n";
    ofstream ofs("delayPhyInfo.txt");
    writeFile(ofs);
    ofs.close();
    cout<<"End\n";
    return 0;
}
