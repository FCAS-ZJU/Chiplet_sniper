#include<iostream>
#include<random>
#include<cstdint>
#include<cstdlib>
#include<cmath>
#include<fstream>
using namespace std;

//芯粒数量，每块芯粒的核心数量，每周期发包数（跟popnet的周期不一样），周期长度，总时间
int main(int argc,char**argv)
{
    if(argc!=6){
        cout<<"Wrong number of parameter\n";
        return 1;
    }
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<int64_t>disChiplet(0,atol(argv[1])-1),disCore(0,atol(argv[2])-1);
    uint64_t cur,t=atol(argv[5]);
    int i,cnt=atoi(argv[3]);
    uint64_t total=0;
    uint64_t period=atol(argv[4]);
    ofstream ofs("physicalRecords.txt");
    for(cur=1;cur<=t;cur+=period){
        for(i=0;i<cnt;++i){
            //发送时间（纳秒） 接收时间（纳秒） 源芯粒 源核心 目的芯粒 目的核心
            ofs<<cur<<' '<<0<<' '
                <<disChiplet(gen)<<' '<<disCore(gen)<<' '
                <<disChiplet(gen)<<' '<<disCore(gen)<<'\n';
            total++;
        }
    }
    ofs.flush();
    ofs.close();
    cout<<"Total packages: "<<total<<endl;
    cout<<"Finished\n";
    return 0;
}
