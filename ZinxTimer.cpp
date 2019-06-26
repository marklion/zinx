#include "ZinxTimer.h"
#include <sys/timerfd.h>

using namespace std;

ZinxTimerChannel::ZinxTimerChannel()
{
}


ZinxTimerChannel::~ZinxTimerChannel()
{
}

/*创建定时器文件描述符*/
bool ZinxTimerChannel::Init()
{
	bool bRet = false;
	/*创建文件描述符*/
	int iFd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (0 <= iFd)
	{
		/*设置定时周期*/
		struct itimerspec period = {
			{1,0}, {1,0}
		};
		if (0 == timerfd_settime(iFd, 0, &period, NULL))
		{
			bRet = true;
			m_TimerFd = iFd;
		}
	}

	return bRet;
}

/*读取超时次数*/
bool ZinxTimerChannel::ReadFd(std::string & _input)
{
	bool bRet = false;
	char buff[8] = { 0 };

	if (sizeof(buff) == read(m_TimerFd, buff, sizeof(buff)))
	{
		bRet = true;
		_input.assign(buff, sizeof(buff));
	}
	return bRet;
}

bool ZinxTimerChannel::WriteFd(std::string & _output)
{
	return false;
}
/*关闭文件描述符*/
void ZinxTimerChannel::Fini()
{
	close(m_TimerFd);
	m_TimerFd = -1;
}
/*返回当前的定时器文件描述符*/
int ZinxTimerChannel::GetFd()
{
	return m_TimerFd;
}

std::string ZinxTimerChannel::GetChannelInfo()
{
	return "TimerFd";
}

class output_hello :public AZinxHandler {
	// 通过 AZinxHandler 继承
	virtual IZinxMsg * InternelHandle(IZinxMsg & _oInput) override
	{
		auto pchannel = ZinxKernel::Zinx_GetChannel_ByInfo("stdout");
		std::string output = "hello world";
		ZinxKernel::Zinx_SendOut(output, *pchannel);
		return nullptr;
	}
	virtual AZinxHandler * GetNextHandler(IZinxMsg & _oNextMsg) override
	{
		return nullptr;
	}
} *pout_hello = new output_hello();

/*返回处理超时事件的对象*/
AZinxHandler * ZinxTimerChannel::GetInputNextStage(BytesMsg & _oInput)
{
	return &TimerOutMng::GetInstance();
}
TimerOutMng TimerOutMng::single;
TimerOutMng::TimerOutMng()
{
	/*创建10个齿*/
	for (int i = 0; i < 10; i++)
	{
		list<TimerOutProc *> tmp;
		m_timer_wheel.push_back(tmp);
	}
}
IZinxMsg * TimerOutMng::InternelHandle(IZinxMsg & _oInput)
{
	unsigned long iTimeoutCount = 0;
	GET_REF2DATA(BytesMsg, obytes, _oInput);
	obytes.szData.copy((char *)&iTimeoutCount, sizeof(iTimeoutCount), 0);

	while (iTimeoutCount-- > 0)
	{
		/*移动刻度*/
		cur_index++;
		cur_index %= 10;
		list<TimerOutProc *> m_cache;
		/*遍历当前刻度所有节点，指向处理函数或圈数-1，*/
		for (auto itr = m_timer_wheel[cur_index].begin(); itr != m_timer_wheel[cur_index].end(); )
		{
			if ((*itr)->iCount <= 0)
			{
				/*缓存待处理的超时节点*/
				m_cache.push_back(*itr);
				auto ptmp = *itr;
				itr = m_timer_wheel[cur_index].erase(itr);
				AddTask(ptmp);
			}
			else
			{
				(*itr)->iCount--;
				++itr;
			}
		}

		/*统一待处理超时任务*/
		for (auto task : m_cache)
		{
			task->Proc();
		}
	}
	
	return nullptr;
}

AZinxHandler * TimerOutMng::GetNextHandler(IZinxMsg & _oNextMsg)
{
	return nullptr;
}

void TimerOutMng::AddTask(TimerOutProc * _ptask)
{
	/*计算当前任务需要放到哪个齿上*/
	int index = (_ptask->GetTimeSec() + cur_index) % 10;
	/*把任务存到该齿上*/
	m_timer_wheel[index].push_back(_ptask);
	/*计算所需圈数*/
	_ptask->iCount = _ptask->GetTimeSec() / 10;
}

void TimerOutMng::DelTask(TimerOutProc * _ptask)
{
	/*遍历时间轮所有齿，删掉任务*/
	for (list<TimerOutProc *> &chi : m_timer_wheel)
	{
		for (auto task : chi)
		{
			if (task == _ptask)
			{
				chi.remove(_ptask);
				return;
			}
		}
	}
}
