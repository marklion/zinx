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
    /*������ʱ���ļ��������ö�ʱ���ڣ������źŵ�Ƶ�ʣ�*/
    virtual bool Init();
    /*��ȡ��ʱ��������ʱ���ļ������������źţ��ͻ�ִ��һ�ν�ʱ���ļ�����������
    ��ȡ��_input��,��Ϊ��Щ����������0.5s�������ź�������1s
    ��Ȼ����û��0.5��ִ��һ�κ������������ǿ��Զ�ȡ�����ĳ�ʱ������
    ���һ�������*/
    virtual bool ReadFd(std::string& _input);
    virtual bool WriteFd(std::string& _output);
    /*�رն�ʱ���ļ�*/
    virtual void Fini();
    /*���ʱ���ļ�������*/
    virtual int GetFd();

    virtual std::string GetChannelInfo();
    /*���ش���ʱ�¼������ߵĶ���
    �������epoll�Ŀ�ܣ��������ź�ʱ�Զ����ں�ִ�з��ض����ڵĺ���*/
    virtual AZinxHandler* GetInputNextStage(BytesMsg& _oInput);
};


class TimerOutProc {
public:
    /*�����źŴ�����*/
    virtual void Proc() = 0;
    /*��ú����õ�ǰ�¼���������*/
    virtual int GetTimeSec() = 0;
    /*��ʣȦ��*/
    int iCount = -1;
};

class TimerOutMng :public AZinxHandler {
    /*�źŴ�������Ϣ������*/
    std::vector<std::list<TimerOutProc*>> m_timer_wheel;
    int cur_index =0;
    /*����ģʽ�����źŴ�����*/
    static TimerOutMng single;
    TimerOutMng();
public:
    /*ÿ��һ�����úõĵ�ʱ���źţ����úõĳ�����ָ��ת��һ���̶ȣ�
    ��ִ�п̶�ָ��ģ��¼������У�Ȧ��<=0���¼����ȵ������ڵ��¼���
    ִ�кú󣬽����¼��ڿ̶�������ɾ�����������¼����°�ʱ������
    ��֮ǰɾ���Ŀ̶ȣ����¼��ص��µĶ�Ӧ�ĳ������¿̶ȵĵ�������*/
    virtual IZinxMsg* InternelHandle(IZinxMsg& _oInput)override;
    /*��û���˵��������һ���źŴ�����*/
    virtual AZinxHandler* GetNextHandler(IZinxMsg& _oNextMsg)override;
    /*��ʱ���źŹ���������¼������¼�����������¼�*/
    void AddTask(TimerOutProc* _ptask);
    /*��ʱ���źŹ�����ɾ���¼�*/
    void DelTask(TimerOutProc* _ptask);
    /*ʱ����������ӿڣ���ö���ĺ���*/
    static TimerOutMng &GetInstance() {
        return single;
    }
};