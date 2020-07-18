#include "syscall_modeling.h"
#include "sift_assert.h"
#include "globals.h"
#include "threads.h"

#include <iostream>
#include <unistd.h>
#include <syscall.h>

//changed at 2020-4-2
#include "../../changes/sniper_change.h"

//什么都没有，只能用文件了
//套接字也行？
//I use socket to communicate
#include <unordered_map>
#include <sys/msg.h>
#include <cstdio>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstddef>
//#include<thread>
#include <pthread.h>
#include <deque>
#include <algorithm>
#include <iterator>
#include <list>
#include <cstdint>
#include <sstream>
#include <string>
#include <iterator>
#include <errno.h>
// int connect_zmq(const std::string&address)
// {
//    return connectZmq(address.c_str());
// }
struct SMsg
{
   int64_t srcAddr, srcPort, desAddr, desPort;
   int64_t datum;
};
struct SConnection
{
   int64_t localAddr, localPort, remoteAddr, remotePort;
   std::list<SMsg> msgs;
};
typedef std::unordered_map<int64_t, SConnection> bufMap_t;
const int SNIPER_PORT_BASE = 7000;
const size_t BUF_SIZE = 4096;
FILE *pro = NULL;
int subnetId;
int sock;
//pthread_mutex_t socketMutex;
PIN_MUTEX socketMutex;
std::deque<std::string> lines;
bufMap_t bufMap;
std::list<SMsg> notClassifiedMsg;
bool isCorrectMsg(const SConnection &conn, const SMsg &msg)
{
   return conn.localAddr == msg.desAddr &&
          conn.localPort == msg.desPort &&
          conn.remoteAddr == msg.srcAddr &&
          conn.remotePort == msg.srcPort;
}
int openZmqProcess()
{
   const char ZMQ_EXE[] = "changes/build/zmq_pro";
   pro = popen(ZMQ_EXE, "r");
   if (pro == NULL)
      return -1;
   fscanf(pro, "%d", &subnetId);
   std::cout << "[CHANGE] get subnet id: " << subnetId << std::endl;
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock < 0)
      return sock;
   sockaddr_in serverAddr;
   memset(&serverAddr, 0, sizeof(serverAddr));
   serverAddr.sin_family = AF_INET;
   serverAddr.sin_port = htons(SNIPER_PORT_BASE + subnetId);
   int r = inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
   if (r <= 0)
      return -1;
   r = connect(sock, (sockaddr *)&serverAddr, sizeof(serverAddr));
   if (r < 0)
      return r;
   //fscanf(pro,"%s",NULL);
   //pthread_mutex_init(&socketMutex,NULL);
   PIN_MutexInit(&socketMutex);
   std::cout << "[CHANGE] connect successfully" << std::endl;
   return 0;
}
int closeZmqProcess()
{
   if (pro == NULL)
      return -1;
   else
   {
      PIN_MutexFini(&socketMutex);
      int r = close(sock);
      if (r < 0)
         return r;
      return pclose(pro);
   }
}
int connectInter(int64_t localPort, int64_t remoteAddr, int64_t remotePort)
{
   bufMap_t::iterator it = bufMap.find(localPort);
   if (it != bufMap.end())
      return -1; //端口被占用
   SConnection connection;
   connection.localAddr = subnetId;
   connection.localPort = localPort;
   connection.remoteAddr = remoteAddr;
   connection.remotePort = remotePort;
   bufMap.insert(std::pair<int64_t, SConnection>(localPort, connection));
   it = bufMap.find(localPort);
   std::list<SMsg>::iterator msgIt, nextMsgIt;
   for (msgIt = notClassifiedMsg.begin(); msgIt != notClassifiedMsg.end(); msgIt = nextMsgIt)
   {
      nextMsgIt = msgIt;
      nextMsgIt++;
      if (isCorrectMsg(it->second, *msgIt))
      {
         it->second.msgs.push_back(*msgIt);
         notClassifiedMsg.erase(msgIt);
      }
   }
   return 0;
}
int disconnectInter(int64_t localPort)
{
   return ((int)bufMap.erase(localPort)) - 1; //移除数只能为1或0。0表示不存在，移除失败；1表示存在，移除成功。
}
/* int receiveLine()
{
   using namespace std;
    static int remain=0;
    static char buf[BUF_SIZE]={0};
    int&connfd=sock;
    if(lines.empty()){
        char*end=buf+remain;
        char*pos=end;
        int r;
        while(pos==end){
			   //不知道为何找不到符号
            r=recv(connfd,end,BUF_SIZE-remain,0);
			   //r=recvfrom(connfd,end,BUF_SIZE-remain,0,NULL,NULL);
            if(r<=0)return r;
            remain+=r;
            end=buf+remain;
            pos=find(buf,end,'\n');
            assert((size_t)remain<BUF_SIZE);
        }
        *pos='\0';
        lines.push_back(buf);
        char*pos0=pos+1;
        for(;;){
            pos=find(pos0,end,'\n');
            if(pos==end)break;
            *pos='\0';
            lines.push_back(pos0);
            pos0=pos+1;
        }
        remain=distance(pos0,end);
        copy(pos0,end,buf);
        return lines.size();
    }
    return 0;
}
int readAllMsg()
{
   int r=receiveLine();
   if(r<=0)return r;
   SMsg t;
   bufMap_t::iterator it;
   for(auto&line:lines){
      //将消息存入不同socket的buf上
      //目的地址 目的端口 源地址 源端口 数据（int 64）
      std::stringstream ss(line);
      ss>>t.desAddr>>t.desPort>>t.srcAddr>>t.srcPort>>t.datum;
      it=bufMap.find(t.desPort);
      if(it!=bufMap.end()&&isCorrectMsg(it->second,t))it->second.msgs.push_back(t);
	  else notClassifiedMsg.push_back(t);
   }
   r=(int)lines.size();
   lines.clear();
   return r;
} */
int sendLine(const std::string &str)
{
   //这个又能找到？？？
   return send(sock, str.c_str(), str.size(), 0);
}
int readMsgUntil(int64_t localPort)
{
   SMsg msg;
   int cnt = 0;
   if(sendLine("recv\n")==-1)return -1;
   //目的地址 目的端口 源地址 源端口 数据（int 64）
   while (fscanf(pro, "%ld%ld%ld%ld%ld",
                 &msg.desAddr,
                 &msg.desPort,
                 &msg.srcAddr,
                 &msg.srcPort,
                 &msg.datum) != EOF)
   {
      if (msg.desAddr == subnetId)
      {
         bufMap_t::iterator it = bufMap.find(msg.desPort);
         if (it != bufMap.end() && isCorrectMsg(it->second, msg))
            it->second.msgs.push_back(msg);
         else
            notClassifiedMsg.push_back(msg);
         cnt++;
      }
      else
         continue;
      if (msg.desPort == localPort)
         break;
      if(sendLine("recv\n")==-1)return -1;
   }
   return cnt;
}
int readMsg(int64_t localPort, int64_t &datum)
{
   bufMap_t::iterator it = bufMap.find(localPort);
   if (it == bufMap.end())
      return -1;
   if (it->second.msgs.empty())
   {
      //if(readAllMsg()<=0)return -1;
      if (readMsgUntil(localPort) <= 0)
         return -1;
   }
   datum = it->second.msgs.front().datum;
   it->second.msgs.pop_front();
   return 0;
}
int writeMsg(const SMsg &msg)
{
   using namespace std;
   /* return sendLine(
      to_string(msg.desAddr)+' '+
      to_string(msg.desPort)+' '+
      to_string(msg.srcAddr)+' '+
      to_string(msg.srcPort)+' '+
      to_string(msg.datum)+'\n'); */
   if (bufMap.find(msg.srcPort) == bufMap.end())
      return -1;
   string str;
   stringstream ss;
   ss << msg.desAddr << ' '
      << msg.desPort << ' '
      << msg.srcAddr << ' '
      << msg.srcPort << ' '
      << msg.datum << '\n';
   getline(ss, str);
   int r=sendLine("send " + str + '\n');
   char strRet[100];
   fscanf(pro,"%s",strRet);
   return r;
}
int lockInter()
{
   //return pthread_mutex_lock(&socketMutex);
   PIN_MutexLock(&socketMutex);
   return 0;
}
int unlockInter()
{
   //return pthread_mutex_unlock(&socketMutex);
   PIN_MutexUnlock(&socketMutex);
   return 0;
}
int getMsgSentFromLocalPort(int64_t localPort, SMsg &msg, int64_t datum)
{
   bufMap_t::iterator it = bufMap.find(localPort);
   if (it == bufMap.end())
      return -1;
   msg.datum = datum;
   msg.srcAddr = subnetId;
   msg.srcPort = localPort;
   msg.desAddr = it->second.remoteAddr;
   msg.desPort = it->second.remotePort;
   return 0;
}

bool handleAccessMemory(void *arg, Sift::MemoryLockType lock_signal, Sift::MemoryOpType mem_op, uint64_t d_addr, uint8_t *data_buffer, uint32_t data_size)
{
   // Lock memory globally if requested
   // This operation does not occur very frequently, so this should not impact performance
   if (lock_signal == Sift::MemLock)
   {
      PIN_GetLock(&access_memory_lock, 0);
   }

   if (mem_op == Sift::MemRead)
   {
      // The simulator is requesting data from us
      PIN_SafeCopy(data_buffer, reinterpret_cast<void *>(d_addr), data_size);
   }
   else if (mem_op == Sift::MemWrite)
   {
      // The simulator is requesting that we write data back to memory
      PIN_SafeCopy(reinterpret_cast<void *>(d_addr), data_buffer, data_size);
   }
   else
   {
      std::cerr << "Error: invalid memory operation type" << std::endl;
      return false;
   }

   if (lock_signal == Sift::MemUnlock)
   {
      PIN_ReleaseLock(&access_memory_lock);
   }

   return true;
}

//commented at 2020-4-2
//真正的系统调用处理函数
//real syscall processing function, which processes all syscalls
// Emulate all system calls
// Do this as a regular callback (versus syscall enter/exit functions) as those hold the global pin lock
VOID emulateSyscallFunc(THREADID threadid, CONTEXT *ctxt)
{
   // If we haven't set our tid yet, do this now
   if (thread_data[threadid].should_send_threadinfo)
   {
      thread_data[threadid].should_send_threadinfo = false;

      Sift::EmuRequest req;
      Sift::EmuReply res;
      req.setthreadinfo.tid = syscall(__NR_gettid);
      thread_data[threadid].output->Emulate(Sift::EmuTypeSetThreadInfo, req, res);
   }

   ADDRINT syscall_number = PIN_GetContextReg(ctxt, REG_GAX);
   //changed at 2020-4-2
   //std::cout<<"Real Syscall: "<<syscall_number<<std::endl;
   sift_assert(syscall_number < MAX_NUM_SYSCALLS);

   syscall_args_t args;
#if defined(TARGET_IA32)
   args[0] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GBX);
   args[1] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GCX);
   args[2] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GDX);
   args[3] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GSI);
   args[4] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GDI);
   args[5] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GBP);
#elif defined(TARGET_INTEL64)
   args[0] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GDI);
   args[1] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GSI);
   args[2] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_GDX);
   args[3] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_R10);
   args[4] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_R8);
   args[5] = PIN_GetContextReg(ctxt, LEVEL_BASE::REG_R9);
#else
#error "Unknown target architecture, require either TARGET_IA32 or TARGET_INTEL64"
#endif

   if (thread_data[threadid].icount_reported > 0)
   {
      thread_data[threadid].output->InstructionCount(thread_data[threadid].icount_reported);
      thread_data[threadid].icount_reported = 0;
   }

   // Default: not emulated, override later when needed
   thread_data[threadid].last_syscall_emulated = false;

   if (syscall_number == SYS_write && thread_data[threadid].output)
   {
      int fd = (int)args[0];
      const char *buf = (const char *)args[1];
      size_t count = (size_t)args[2];

      if (count > 0 && (fd == 1 || fd == 2))
         thread_data[threadid].output->Output(fd, buf, count);
   }

   if (KnobEmulateSyscalls.Value() && thread_data[threadid].output)
   {
      //changed at 2020-4-2
      //std::cout<<"Syscall switch: "<<syscall_number<<std::endl;
      switch (syscall_number)
      {
      // Handle SYS_clone child tid capture for proper pthread_join emulation.
      // When the CLONE_CHILD_CLEARTID option is enabled, remember its child_tidptr and
      // then when the thread ends, write 0 to the tid mutex and futex_wake it
      case SYS_clone:
      {
         if (args[0] & CLONE_THREAD)
         {
// Store the thread's tid ptr for later use
#if defined(TARGET_IA32)
            ADDRINT tidptr = args[2];
#elif defined(TARGET_INTEL64)
            ADDRINT tidptr = args[3];
#endif
            PIN_GetLock(&new_threadid_lock, threadid);
            tidptrs.push_back(tidptr);
            PIN_ReleaseLock(&new_threadid_lock);
            /* New thread */
            thread_data[threadid].output->NewThread();
         }
         else
         {
            /* New process */
            // Nothing to do there, handled in fork()
         }
         break;
      }

      // System calls not emulated (passed through to OS)
      case SYS_read:
      case SYS_write:
      case SYS_wait4:
         thread_data[threadid].last_syscall_number = syscall_number;
         thread_data[threadid].last_syscall_emulated = false;
         thread_data[threadid].output->Syscall(syscall_number, (char *)args, sizeof(args));
         break;

      // System calls emulated (not passed through to OS)
      case SYS_futex:
      case SYS_sched_yield:
      case SYS_sched_setaffinity:
      case SYS_sched_getaffinity:
      case SYS_nanosleep:
         thread_data[threadid].last_syscall_number = syscall_number;
         thread_data[threadid].last_syscall_emulated = true;
         thread_data[threadid].last_syscall_returnval = thread_data[threadid].output->Syscall(syscall_number, (char *)args, sizeof(args));
         break;

      // System calls sent to Sniper, but also passed through to OS
      case SYS_exit_group:
         thread_data[threadid].output->Syscall(syscall_number, (char *)args, sizeof(args));
         break;

      //changed at 2020-4-2
      //插入跨芯粒读写的系统调用号
      //syscall number on inter-chiplet operation
      case nsChange::SYSCALL_TEST_CHANGE:
      {
         //测试
         std::cout << "Test succeeds(recorder)\n";
         //test reg func
         CALL_APPLICATION_FUNCTION_PARAM p;
         p.native = 1;
         PIN_CallApplicationFunction(
             ctxt,
             threadid,
             CALLINGSTD_DEFAULT,
             /*thread_data[threadid].funcReg*/ regFunc[thread_data[threadid].thread_num].testFunc,
             &p,
             PIN_PARG_END());
         thread_data[threadid].last_syscall_number = syscall_number;
         thread_data[threadid].last_syscall_emulated = true;
         thread_data[threadid].last_syscall_returnval = thread_data[threadid].output->Syscall(
             syscall_number,
             (char *)args,
             sizeof(args));
         break;
      }

      case nsChange::SYSCALL_REMOTE_READ:
      {
         //TODO: 跨芯粒读
         //本地端口，缓冲区地址
         //lockInter();
         thread_data[threadid].last_syscall_number = syscall_number;
         lockInter();
         int64_t datum;
         thread_data[threadid].last_syscall_emulated = (readMsg(args[0], datum) == 0);
         //unlockInter();
         /* CALL_APPLICATION_FUNCTION_PARAM p;
         p.native = 1;
         PIN_CallApplicationFunction(
             ctxt,
             threadid,
             CALLINGSTD_DEFAULT,
             regFunc[thread_data[threadid].thread_num].writeProMem,
             &p,
             PIN_PARG(uintptr_t),
             args[1],
             PIN_PARG(int64_t),
             datum,
             PIN_PARG_END()); */
         unlockInter();
         /* thread_data[threadid].last_syscall_returnval = thread_data[threadid].output->Syscall(
             syscall_number,
             (char *)args,
             sizeof(args)); */
         thread_data[threadid].last_syscall_returnval=datum;
         thread_data[threadid].output->Syscall(syscall_number,(char*)args,sizeof(args));
            //unlockInter();
         break;
      }

      case nsChange::SYSCALL_REMOTE_WRITE:
      {
         //TODO: 跨芯粒写
         //本地端口，数据值
         //lockInter();
         //std::cout<<args[0]<<' '<<args[1]<<std::endl;
         thread_data[threadid].last_syscall_number = syscall_number;
         SMsg msg;
         lockInter();
         int r = getMsgSentFromLocalPort(args[0], msg, args[1]);
         //std::cout<<r<<std::endl;
         if (r < 0)
            thread_data[threadid].last_syscall_emulated = false;
         else
         {
            r = writeMsg(msg);
            /* std::cout<<"writeMsg: "<<r<<std::endl;
				std::cout<<"errno: "<<errno<<std::endl; */
            thread_data[threadid].last_syscall_emulated = (r > 0);
         }
         unlockInter();
         thread_data[threadid].last_syscall_returnval = thread_data[threadid].output->Syscall(
             syscall_number,
             (char *)args,
             sizeof(args));
         //unlockInter();
         break;
      }

      case nsChange::SYSCALL_REG_FUNC:
      {
         //注册函数
         //已弃用
         thread_data[threadid].last_syscall_number = syscall_number;
         thread_data[threadid].last_syscall_emulated = true;
         std::cout << "reg func: " << args[0] << std::endl;
         //thread_data[threadid].funcReg=(AFUNPTR)args[0];
         regFunc[thread_data[threadid].thread_num].testFunc = (AFUNPTR)args[0];
         thread_data[threadid].last_syscall_returnval = thread_data[threadid].output->Syscall(
             syscall_number,
             (char *)args,
             sizeof(args));
         break;
      }

      case nsChange::SYSCALL_CONNECT:
      {
         //建立连接
         //lockInter();
         thread_data[threadid].last_syscall_number = syscall_number;
         lockInter();
         //本地端口，远程地址，远程端口
         thread_data[threadid].last_syscall_emulated = (connectInter(args[0], args[1], args[2]) == 0);
         unlockInter();
         thread_data[threadid].last_syscall_returnval = thread_data[threadid].output->Syscall(
             syscall_number,
             (char *)args,
             sizeof(args));
         //unlockInter();
         break;
      }

      case nsChange::SYSCALL_DISCONNECT:
      {
         //结束连接
         //lockInter();
         thread_data[threadid].last_syscall_number = syscall_number;
         lockInter();
         //本地端口
         thread_data[threadid].last_syscall_emulated = (disconnectInter(args[0]) == 0);
         unlockInter();
         thread_data[threadid].last_syscall_returnval = thread_data[threadid].output->Syscall(
             syscall_number,
             (char *)args,
             sizeof(args));
         //unlockInter();
         break;
      }
      case nsChange::SYSCALL_GET_LOCAL_ADDR:
      {
         //获得本机地址
         thread_data[threadid].last_syscall_number = syscall_number;
         thread_data[threadid].last_syscall_emulated = true;
         thread_data[threadid].last_syscall_returnval = subnetId;
         thread_data[threadid].output->Syscall(
             syscall_number,
             (char *)args,
             sizeof(args));
      }
      }
   }
}

static VOID syscallEntryCallback(THREADID threadid, CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard, VOID *v)
{
   if (!thread_data[threadid].last_syscall_emulated)
   {
      return;
   }

   //changed at 2020-4-19
   //std::cout<<"add syscall numbers\n";
   //是这里
   //it is here
   PIN_SetSyscallNumber(ctxt, syscall_standard, SYS_getpid);
   /* PIN_SetSyscallNumber(ctxt, syscall_standard, nsChange::SYSCALL_TEST_CHANGE);
   PIN_SetSyscallNumber(ctxt, syscall_standard, nsChange::SYSCALL_REMOTE_READ);
   PIN_SetSyscallNumber(ctxt, syscall_standard, nsChange::SYSCALL_REMOTE_WRITE);
   PIN_SetSyscallNumber(ctxt, syscall_standard, nsChange::SYSCALL_REG_FUNC); */
}

static VOID syscallExitCallback(THREADID threadid, CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard, VOID *v)
{
   //changed at 2020-4-19
   //std::cout<<"syscall return: "<<PIN_GetSyscallReturn(ctxt,syscall_standard)<<std::endl;
   if (!thread_data[threadid].last_syscall_emulated)
   {
      return;
   }

   //changed at 2020-4-19
   //std::cout<<"syscall return: "<<PIN_GetSyscallReturn(ctxt,syscall_standard)<<std::endl;
   //changed at 2020-5-1
   ADDRINT syscallRet;
   syscallRet = thread_data[threadid].last_syscall_returnval;
   PIN_SetContextReg(ctxt, REG_GAX, syscallRet);
   thread_data[threadid].last_syscall_emulated = false;
}

void initSyscallModeling()
{
   //changed at 2020-4-19
   //std::cout<<"init syscall model\n";
   PIN_AddSyscallEntryFunction(syscallEntryCallback, 0);
   PIN_AddSyscallExitFunction(syscallExitCallback, 0);
}
