#include<iostream>
#include<cmath>
#include<fstream>
#include<cstdint>
#include<cstdlib>
#include<array>
using namespace std;

int64_t blockDistance(int64_t x1,int64_t y1,int64_t x2,int64_t y2)
{
    return abs(x1-x2)+abs(y1-y2);
}

//芯粒间
const size_t CHIPLET_KIND=3;
array<int64_t,CHIPLET_KIND>chipletHopSums={0};
size_t chipletType(int64_t chipletX,int64_t chipletY)
{
    //返回种类
    return (chipletX==1||chipletX==2)?0:1;
}
void addInterchipletDistance(int64_t chipletX,int64_t chipletY)
{
    //分种类加
    chipletHopSums[chipletType(chipletX,chipletY)]++;
}

//轨迹文件
int main(int argc,char**argv)
{
     if(argc!=2){
        cout<<"Wrong number of parameters\n";
        return 1;
    }
    ifstream ifs(argv[1]);
    double t;
    int64_t srcChipletX,srcChipletY,srcCoreX,srcCoreY,desChipletX,desChipletY,desCoreX,desCoreY;
    uint32_t s;
    int64_t chipletHopSum=0,coreHopSum=0;
    int64_t chipletHopSumX=0,chipletHopSumY=0;
    int64_t chipletHop;
    while(ifs>>t
        >>srcChipletX
        >>srcChipletY
        >>srcCoreX
        >>srcCoreY
        >>desChipletX
        >>desChipletY
        >>desCoreX
        >>desCoreY>>s){
            chipletHop=blockDistance(desChipletX,desChipletY,srcChipletX,srcChipletY);
            if(chipletHop==0){
                //芯粒内
                coreHopSum+=blockDistance(desCoreX,desCoreY,srcCoreX,srcCoreY);
            }else{
                //芯粒间
                chipletHopSum+=chipletHop;
                coreHopSum+=blockDistance(desCoreX,desCoreY,0,0)+blockDistance(0,0,srcCoreX,srcCoreY);
                /* chipletHopSumX+=abs(desChipletX-srcChipletX);
                chipletHopSumY+=abs(desChipletY-srcChipletY); */
                addInterchipletDistance(srcChipletX,srcChipletY);
                addInterchipletDistance(desChipletX,desChipletY);
            }
    }
    cout<<"Inside-chiplet hops: "<<coreHopSum<<endl;
    for(size_t i=0;i<CHIPLET_KIND;++i){
        cout<<"Inter-chiplet Type "<<i<<" hops: "<<chipletHopSums[i]<<endl;
    }
    return 0;
}
