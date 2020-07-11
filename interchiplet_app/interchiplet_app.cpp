#include"interchiplet_app.h"

#include"interchiplet_app_core.h"
#include"sniper_change.h"

#include<syscall.h>
#include<unistd.h>
#include<mutex>

using namespace std;
using namespace nsChange;

namespace nsInterchiplet
{
    static mutex mtx;
    syscall_return_t openPort(int64_t localPort,int64_t remoteAddress,int64_t remotePort)
    {
        return syscall(SYSCALL_CONNECT,localPort,remoteAddress,remotePort);
    }
    syscall_return_t closePort(int64_t localPort)
    {
        return syscall(SYSCALL_DISCONNECT,localPort);
    }
    syscall_return_t recvMsgNoLock(int64_t port,int64_t*desPtr)
    {
        syscall_return_t r;
        r=syscall(SYSCALL_REMOTE_READ,port,(uintptr_t)desPtr);
        getRemoteValue();
        return r;
    }
    syscall_return_t receiveMessage(int64_t port,int64_t*desPtr)
    {
        syscall_return_t r;
        /* mtx.lock();
        r=recvMsgNoLock(port,desPtr);
        mtx.unlock(); */
        r=syscall(SYSCALL_REMOTE_READ,port);
        if(desPtr!=NULL&&desPtr!=nullptr)*desPtr=r;
        return r;
    }
    bool receiveMessageNoBlocking(int64_t port,int64_t*desPtr,syscall_return_t*syscallRetPtr)
    {
        bool r;
        r=mtx.try_lock();
        if(r){
            syscall_return_t syscallRet=recvMsgNoLock(port,desPtr);
            mtx.unlock();
            if(syscallRetPtr!=nullptr&&syscallRetPtr!=NULL)*syscallRetPtr=syscallRet;
        }
        return r;
    }
    syscall_return_t sendMessage(int64_t localPort,int64_t datum)
    {
        return syscall(SYSCALL_REMOTE_WRITE,localPort,datum);
    }
    syscall_return_t getLocalAddress()
    {
        return syscall(SYSCALL_GET_LOCAL_ADDR);
    }
}
