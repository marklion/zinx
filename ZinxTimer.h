#pragma once
#include<vector>
#include <zinx.h>
#include<list>

class ZinxTimerChannel :
    public Ichannel
{
    int m_TimerFd = -1;
public:
    ZinxTimerChannel();
    virtual ~ZinxTimerChannel();
    /*创建定时器文件，并设置定时周期（产生信号的频率）*/
    virtual bool Init();
    /*读取超时次数，当时间文件描述符产生信号，就会执行一次将时间文件描述符内容
    读取到_input中,因为有些函数周期是0.5s，但是信号周期是1s
    虽然我们没法0.5秒执行一次函数，但是我们可以读取到他的超时次数，
    变成一秒读两次*/
    virtual bool ReadFd(std::string& _input);
    virtual bool WriteFd(std::string& _output);
    /*关闭定时器文件*/
    virtual void Fini();
    /*获得时间文件描述符*/
    virtual int GetFd();

    virtual std::string GetChannelInfo();
    /*返回处理超时事件管理者的对象
    框架类似epoll的框架，当产生信号时自动在内核执行返回对象内的函数*/
    virtual AZinxHandler* GetInputNextStage(BytesMsg& _oInput);
};


class TimerOutProc {
public:
    /*设置信号处理函数*/
    virtual void Proc() = 0;
    /*获得和设置当前事件处理周期*/
    virtual int GetTimeSec() = 0;
    /*所剩圈数*/
    int iCount = -1;
};

class TimerOutMng :public AZinxHandler {
    /*信号处理函数信息的链表*/
    std::vector<std::list<TimerOutProc*>> m_timer_wheel;
    int cur_index =0;
    /*单例模式管理信号处理函数*/
    static TimerOutMng single;
    TimerOutMng();
public:
    /*每过一个设置好的的时间信号，设置好的齿轮盘指针转动一个刻度，
    并执行刻度指向的，事件链表中，圈数<=0的事件，既到达周期的事件，
    执行好后，将该事件在刻度链表上删除，并将该事件重新按时间周期
    和之前删除的刻度，重新加载到新的对应的齿轮盘新刻度的的链表中*/
    virtual IZinxMsg* InternelHandle(IZinxMsg& _oInput)override;
    /*获得或者说是设置下一个信号处理函数*/
    virtual AZinxHandler* GetNextHandler(IZinxMsg& _oNextMsg)override;
    /*给时间信号管理者添加事件，在事件齿轮上添加事件*/
    void AddTask(TimerOutProc* _ptask);
    /*给时间信号管理者删除事件*/
    void DelTask(TimerOutProc* _ptask);
    /*时间管理类对外接口，获得对象的函数*/
    static TimerOutMng &GetInstance() {
        return single;
    }
};