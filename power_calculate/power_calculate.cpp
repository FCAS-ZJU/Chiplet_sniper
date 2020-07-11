#include<iostream>
#include<cmath>
#include<fstream>
#include<cstdint>
#include<cstdlib>
using namespace std;

/* template<typename T>T blockDistance(T x1,T y1,T x2,T y2)
{
    return abs(x1-x2)+abs(y1-y2);
} */

int64_t blockDistance(int64_t x1,int64_t y1,int64_t x2,int64_t y2)
{
    return abs(x1-x2)+abs(y1-y2);
}

//轨迹文件，芯粒间每跳功耗，芯粒内每跳功耗
int main(int argc,char**argv)
{
    if(argc!=2&&argc!=4){
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
                chipletHopSumX+=abs(desChipletX-srcChipletX);
                chipletHopSumY+=abs(desChipletY-srcChipletY);
            }
    }
    cout<<"Inter-chiplet hops: "<<chipletHopSum<<endl
        <<"Inside-chiplet hops: "<<coreHopSum<<endl;
    cout<<"Inter-chiplet X hops: "<<chipletHopSumX
        <<"\nInter-chiplet Y hops: "<<chipletHopSumY<<endl;
    if(argc==4){
        cout<<"Total hop power: "<<(chipletHopSum*atof(argv[2])+coreHopSum*atof(argv[3]))<<endl;
    }
    return 0;
}
