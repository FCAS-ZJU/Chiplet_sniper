#include<stdint.h>

extern bool changed;
#ifdef __cplusplus
extern"C"{
#endif
    int regFunc(void*funcPtr);

    void change();

    //供其他芯粒读写本地变量之用
    void remoteRead(const char*localAddrName,int64_t offset,int64_t*readValue,bool*successful);
    void remoteWrite(const char*localAddrName,int64_t offset,int64_t writeValue,bool*successful);
    
    void setAddrMap(const char*localAddrName,int64_t*ptr);
    void removeAddrMap(const char*localAddrName);

    void getRemoteValue();
    //删掉该注释vscode报错，可能是bug
#ifdef __cplusplus
}
#endif
