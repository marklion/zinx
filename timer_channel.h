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

	// 通过 Ichannel 继承
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
	/*单例对象指针*/
	static timeout_deliver *m_single;

	/*时间轮*/
	std::vector<std::list<timeout_task *>> m_wheel;
	/*当前指向的刻度*/
	int cur_pos = 0;
public:

	static timeout_deliver *GetInstance();

	// 通过 AZinxHandler 继承
	virtual IZinxMsg * InternelHandle(IZinxMsg & _oInput) override;
	virtual AZinxHandler * GetNextHandler(IZinxMsg & _oNextMsg) override;

	/*提供任务队列的添加删除函数*/
	void register_task(int _sec, timeout_task *_task);
	void unregister_task(timeout_task *_task);
};

