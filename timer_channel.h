#pragma once
#include <zinx.h>
#include <vector>
#include <list>
class timer_channel :
	public Ichannel
{
	int m_fd = -1;
public:
	timer_channel();
	virtual ~timer_channel();

	// ͨ�� Ichannel �̳�
	virtual bool Init() override;
	virtual bool ReadFd(std::string & _input) override;
	virtual bool WriteFd(std::string & _output) override;
	virtual void Fini() override;
	virtual int GetFd() override;
	virtual std::string GetChannelInfo() override;
	virtual AZinxHandler * GetInputNextStage(BytesMsg & _oInput) override;
};

class timeout_task {
public:
	virtual void proc_timeout() = 0;
	int m_count = 0;
	int time_out = 0;
};

class timeout_deliver :
	public AZinxHandler
{
	timeout_deliver();
	virtual ~timeout_deliver();
	/*��������ָ��*/
	static timeout_deliver *m_single;

	/*ʱ����*/
	std::vector<std::list<timeout_task *>> m_wheel;
	/*��ǰָ��Ŀ̶�*/
	int cur_pos = 0;
public:

	static timeout_deliver *GetInstance();

	// ͨ�� AZinxHandler �̳�
	virtual IZinxMsg * InternelHandle(IZinxMsg & _oInput) override;
	virtual AZinxHandler * GetNextHandler(IZinxMsg & _oNextMsg) override;

	/*�ṩ������е����ɾ������*/
	void register_task(int _sec, timeout_task *_task);
	void unregister_task(timeout_task *_task);
};

