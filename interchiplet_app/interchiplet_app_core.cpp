#include"interchiplet_app_core.h"

#include<iostream>
#include<unistd.h>
#include<unordered_map>

using namespace std;

typedef std::unordered_map<std::string,int64_t*> addrMap_t;

extern"C" int regFunc(void*funcPtr)
{
    const int REG_FUNC_SYSCALL_NUM=503;
    return syscall(REG_FUNC_SYSCALL_NUM,funcPtr);
}
bool changed=false;
extern"C" void change()
{
    cout<<"change was called\n";
    changed=true;
}
static addrMap_t addrMap;
//相对本机
extern"C" void remoteRead(const char*localAddrName,int64_t offset,int64_t*readValue,bool*successful)
{
    addrMap_t::iterator it=addrMap.find(localAddrName);
    if(it==addrMap.end())*successful=false;
    else{
        *readValue=(it->second)[offset];
        *successful=true;
    }
}
extern"C" void remoteWrite(const char*localAddrName,int64_t offset,int64_t writeValue,bool*successful)
{
    addrMap_t::iterator it=addrMap.find(localAddrName);
    if(it==addrMap.end())*successful=false;
    else{
        (it->second)[offset]=writeValue;
        *successful=true;
    }
}

extern"C" void setAddrMap(const char*localAddrName,int64_t*ptr)
{
    addrMap[localAddrName]=ptr;
}
extern"C" void removeAddrMap(const char*localAddrName)
{
    addrMap.erase(localAddrName);
}

static int64_t*desPtr=NULL;
static int64_t value=0;
extern"C" void writeProcessMemory0(uintptr_t desPtr0,int64_t value0)
{
    //*(int64_t*)desPtr=value;
    desPtr=(int64_t*)desPtr0;
    value=value0;
}
//参数的顺序是反转的？？？
extern"C" void writeProcessMemory1(int64_t value0,uintptr_t desPtr0)
{
    //*(int64_t*)desPtr=value;
    desPtr=(int64_t*)desPtr0;
    value=value0;
}
extern"C" void writeProcessMemory(int64_t value0,uintptr_t desPtr0)
{
    //cout<<"get pointer: "<<desPtr0<<" value: "<<value0<<endl;
    //*(int64_t*)desPtr0=value0;
    writeProcessMemory1(value0,desPtr0);
    //getRemoteValue();
}
extern"C" void getRemoteValue()
{
    //cout<<"get pointer: "<<(uintptr_t)desPtr<<" value: "<<value<<endl;
    *desPtr=value;
}
