# zinx
simple,pure IO

# Usage:

## 1.Read stdin echo to stdout

```cpp
#include <zinx.h>
#include <iostream>
using namespace std;

/*define class used to srite stdout*/
class TestStdout:public Ichannel{
public:
    /*do nothing*/
    virtual bool Init(){}
    /*do nothing*/
    virtual bool ReadFd(std::string &_input){return false;}
    /*write to STDOUT directly*/
    virtual bool WriteFd(std::string &_output){
        cout << _output <<endl;
        return true;
    }
    /*do nothing*/
    virtual void Fini(){}
    /*return 1 which point to STDOUT*/
    virtual int GetFd(){return 1;}
    /*no impact*/
    virtual std::string GetChannelInfo(){return "test";}
    /*no next stage*/
    virtual AZinxHandler *GetInputNextStage(BytesMsg &_oInput){return NULL;}
} *poOut = new TestStdout();

class Echo:public AZinxHandler
{
public:
    /*define echo action is get string from input, and send out is via stdout channel object*/
    virtual IZinxMsg *InternelHandle(IZinxMsg &_oInput){
        GET_REF2DATA(BytesMsg, oBytes, _oInput);
        ZinxKernel::Zinx_SendOut(oBytes.szData, *poOut);
        return NULL;
    }
    /*no next stage*/
    virtual AZinxHandler *GetNextHandler(IZinxMsg &_oNextMsg){return NULL;}
} *poEcho = new Echo();

class TestStdin:public Ichannel{
public:
    /*do nothing*/
    virtual bool Init(){}
    virtual bool ReadFd(std::string &_input){
        cin>>_input;
        return true;
    }
    /*do nothing*/
    virtual bool WriteFd(std::string &_output){return false;}
    /*do nothing*/
    virtual void Fini(){}
    /*return 0 which point to STDIN*/
    virtual int GetFd(){return 0;}
    /*no impact*/
    virtual std::string GetChannelInfo(){return "test";}
    /*specify next stage is echo handler*/
    virtual AZinxHandler *GetInputNextStage(BytesMsg &_oInput){return poEcho;}
} *poIn = new TestStdin();

/*before main func called, three globle object was created before which were poOut point to a TestStdout object, poEcho point to a Echo object and poIn point to a TestStdin object.*/
int main()
{
    ZinxKernel::ZinxKernelInit();
    /*Add stdin and stdout channel to kernel*/
    ZinxKernel::Zinx_Add_Channel(*poIn);
    ZinxKernel::Zinx_Add_Channel(*poOut);
    /*start loop*/
    ZinxKernel::Zinx_Run();
    ZinxKernel::ZinxKernelFini();
    return 0;
}
```

**process review:**

```
+----------------+               +------------------+
| console input  +-------------> |TestStdin::ReadFd |
+----------------+               +--------+---------+
                                          |
                                          |
                                          |
                                          v
                                +---------+----------+
                                |Echo::InternelHandle|
                                +---------+----------+
                                          |
                                          |
                                          |
                                          |
                                          v
+---------------+               +---------+----------+
| console output+<--------------+TestSTDOut：：WriteFd |
+---------------+               +--------------------+
```
