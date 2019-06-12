# zinx
simple,pure IO

[similar framework in golang](https://github.com/aceld/zinx)

# Description

This is a simple framework handling multi-IO. Include but not limited socket IO was supported. Developer can implement their own action in every stage of a whole IO process via override the methods of abstract classes which were pre-defined in this framework. At the same time, framework also provide several specific classes used to cover almost common function such as TCP socket, STDIN,STDOUT and so on.
We appreciate every developer fork this repository and implement more and more specific classes to solve problem in each difficult situation and layer of data translation.

# Install
1. clone the whole project through ssh or http

    `git clone https://github.com/marklion/zinx.git`

2. enter project folder and run make

    `cd zinx`

    `make`

3. install library and header file.

    `sudo make install`

**Note:** you should add compile flag -std=c++11 and link flag -lzinx when build your own application using this library.

# Quick Start

1. Call ZinxKernel::ZinxKernelInit() first to initialize IO-schedule kernel of this framework.
2. Create a class inherit AZinxHandler and override InternelHandle. In this function you can add your own code in which is used to handle bytes stream read from some input channel. For example, you can just print bytes on STDOUT. When you override this function, you may use the macro GET_REF2DATA to convert IZinxMsg reference to BytesMsg reference.
3. Create a or more classes inherit Ichannel and override the required functions which defined the actions of one specific data channel(file description in general). When you override GetInputNextStage, you were actually pointing who is the next handler. So return a object create based on class you defined in step 2 is the most adviced.
4. Create object based on the Ichannel class you create in step 2, and add it to kernel using ZinxKernel::Zinx_Add_Channel().
5. Call ZinxKernel::Zinx_Run() and your process will enter a loop in which data will be processed obeying method you pre-defined in function InernelHandle when data came from file opened as fd of Ichannel object.

# Sample:

## Read stdin echo to stdout

```cpp
#include <zinx.h>
#include <iostream>
using namespace std;

/*define class used to write stdout*/
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
    /*define echo action which is get string from input, and send out it via stdout channel object*/
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

# Advanced Usage

You should split your whole business into several layer including channel protocol role and so on.

+ In channel layer, you'd better only write your basic IO-process via inherit Ichannel class. For example, maybe you need call a native system-call like read or write when you override the ReadFd and WriteFd function. Parameters of functions above should used to store data ready to send or just arrived.
+ In protocol layer, we suggest you put your data convertion here. You can create one or more object of sub-class based on Iprotocol class to put your several layer data translation protocol in. Most of all, you may need write data verifiction and message deliver by override raw2request and GetMsgProcessor, and you may need write data serializition by override response2raw and GetMsgSender.
+ In role layer, it's the best place to write your pure business code. Function ProcMsg is used to override to process specific user-data(not about IO and protocol). In addition, you can define several objects based on different Irole classes and then link them as a chain through SetNextProcessor.  In this case, a original user-data will be handled by a role-chain one by one.

> For more details of every single function. Please read the code comment directly.

# Utility class

To provide common useful function avoid developer repeat working, this framework has three utility classes implement TCP data channel.

## 1 ZinxTcpData

This class has only one virtual fucntion GetInputNextStage that is the same meaning of its super-class. You should inherit this class and override this function to define which handler should handle received bytes stream.

The objects based on this class maintain their own socket. So the object should be constructed after one client connected.

## 2 IZinxTcpConnFact

As its name, this class is an abstract class which is used to construct ZinxTcpData object. You just need to override the only one virtual function CreateTcpDataChannel to return suitable ZinxTcpData object.

## 3 ZinxTCPListen

This Class is not a abstract class, you can use it to construct object directly. When construction, TCP listen port number and one object of IZinxTcpConnFact sub-class should be specified.

