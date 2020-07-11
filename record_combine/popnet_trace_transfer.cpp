#include<iostream>
#include<fstream>
#include<cstdlib>
#include<cstdint>
#include<vector>
#include<filesystem>
#include<unordered_map>
#include<queue>
#include<string>
using namespace std;

//时间（纳秒） 源芯粒 源核心 目的芯粒 目的核心
struct SMsgPhyRecord{
    uint64_t timeSent;
    int64_t srcChiplet,srcCore,desChiplet,desCore;
};
struct SPopnetRecord{
    double timeSent;
    int64_t srcChipletX,srcChipletY,srcCoreX,srcCoreY;
    int64_t desChipletX,desChipletY,desCoreX,desCoreY;
    uint32_t size;
};
vector<SPopnetRecord>vRecords;
void readAndTransferRecords(ifstream&ifsRecords,int64_t chipletCntPL,int64_t coreCntPL,double freq)
{
    const uint32_t MSG_SIZE=1;
    const double TIME_UNIT_FACTOR=1;
    SMsgPhyRecord r;
    ldiv_t srcChiplet,srcCore,desChiplet,desCore;
    uint64_t timeRecv;
    //发送时间（纳秒） 接收时间（纳秒） 源芯粒 源核心 目的芯粒 目的核心
    while(ifsRecords>>r.timeSent>>timeRecv>>r.srcChiplet>>r.srcCore>>r.desChiplet>>r.desCore){
        srcChiplet=ldiv(r.srcChiplet,chipletCntPL);
        srcCore=ldiv(r.srcCore,coreCntPL);
        desChiplet=ldiv(r.desChiplet,chipletCntPL);
        desCore=ldiv(r.desCore,coreCntPL);
        vRecords.push_back({
            r.timeSent*TIME_UNIT_FACTOR*freq,
            srcChiplet.rem,
            srcChiplet.quot,
            srcCore.rem,
            srcCore.quot,
            desChiplet.rem,
            desChiplet.quot,
            desCore.rem,
            desCore.quot,
            MSG_SIZE
        });
    }
}

struct SPos{
    int64_t chipletX,chipletY,coreX,coreY;
};
struct SHashFunc{
    size_t operator()(const SPos&p)const
    {
        return p.chipletX+p.chipletY+p.coreX+p.coreY;
    }
};
struct SEqualFunc{
    bool operator()(const SPos&a,const SPos&b)const
    {
        return a.chipletX==b.chipletX&&a.chipletY==b.chipletY&&a.coreX==b.coreX&&a.coreY==b.coreY;
    }
};
void outputPopnetTrace(const string&outputDirPath)
{
    const string TRACE_FILE_NAME_0("bench");
    filesystem::create_directory(outputDirPath);
    string traceFileName(outputDirPath+'/'+TRACE_FILE_NAME_0);
    ofstream ofs(traceFileName);
    typedef queue<SPopnetRecord> queue_record_t;
    typedef unordered_map<SPos,queue_record_t,SHashFunc,SEqualFunc> mapSrcTrace_t;
    mapSrcTrace_t mapSrcTrace;
    /* mapSrcTrace_t::iterator it;
    SPos pos; */
    for(auto&r:vRecords){
        //时间（周期），源地址，目的地址，大小
        ofs<<r.timeSent<<' '
            <<r.srcChipletX<<' '<<r.srcChipletY<<' '<<r.srcCoreX<<' '<<r.srcCoreY<<' '
            <<r.desChipletX<<' '<<r.desChipletY<<' '<<r.desCoreX<<' '<<r.desCoreY<<' '
            <<r.size<<'\n';
        mapSrcTrace.try_emplace({
            r.srcChipletX,
            r.srcChipletY,
            r.srcCoreX,
            r.srcCoreY
        }).first->second.push(r);
    }
    vRecords.clear();
    ofs.flush();
    ofs.close();
    string fileName;
    for(auto&q:mapSrcTrace){
        fileName=traceFileName+'.'+
            to_string(q.first.chipletX)+'.'+
            to_string(q.first.chipletY)+'.'+
            to_string(q.first.coreX)+'.'+
            to_string(q.first.coreY);
        ofs.open(fileName);
        while(!q.second.empty()){
            auto&top=q.second.front();
            //时间（周期），源地址，目的地址，大小
            ofs<<top.timeSent<<' '
                <<top.srcChipletX<<' '<<top.srcChipletY<<' '<<top.srcCoreX<<' '<<top.srcCoreY<<' '
                <<top.desChipletX<<' '<<top.desChipletY<<' '<<top.desCoreX<<' '<<top.desCoreY<<' '
                <<top.size<<'\n';
            q.second.pop();
        }
        ofs.flush();
        ofs.close();
    }
}

//芯粒每行数量，芯粒内核心每行数量，频率（GHz）
int main(int argc,char**argv)
{
    if(argc!=4){
        cout<<"Wrong number of parameters\n";
        return 1;
    }
    //芯粒每行数量，芯粒内核心每行数量
    int64_t chipletPerLineCnt=atol(argv[1]),corePerLineCnt=atol(argv[2]);
    //频率，单位为GHz
    double frequency=atof(argv[3]);
    ifstream ifs("physicalRecords.txt");
    cout<<"Reading...\n";
    readAndTransferRecords(ifs,chipletPerLineCnt,corePerLineCnt,frequency);
    ifs.close();
    cout<<"Finish reading. Writing...\n";
    outputPopnetTrace("traces");
    cout<<"End\n";
    return 0;
}
