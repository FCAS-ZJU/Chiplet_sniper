#include<cstdint>
//#include<tuple>
#include<cstddef>

namespace nsInterchiplet
{
    typedef long syscall_return_t;
    syscall_return_t openPort(int64_t localPort,int64_t remoteAddress,int64_t remotePort);
    syscall_return_t closePort(int64_t localPort);
    syscall_return_t receiveMessage(int64_t port,int64_t*desPtr);
    //bool receiveMessageNoBlocking(int64_t port,int64_t*desPtr,syscall_return_t*syscallRetPtr=nullptr);
    syscall_return_t sendMessage(int64_t localPort,int64_t datum);
    syscall_return_t getLocalAddress();
}
