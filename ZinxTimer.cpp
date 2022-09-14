#include "ZinxTimer.h"
#include <sys/timerfd.h>
using namespace std;

ZinxTimerChannel::ZinxTimerChannel()
{}

ZinxTimerChannel::~ZinxTimerChannel()
{}
/*创建定时器文件表*/
bool ZinxTimerChannel::Init()
{
	bool bRet = false;
	/*创建文件描述符*/
	int iFd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (0 <= iFd)
	{
		/*设置定时周期*/
		struct itimerspec period = { {1,0},{1,0} };
		if (0 == timerfd_settime(iFd, 0, &period, NULL))
		{
			bRet = true;
			m_TimerFd = iFd;
		}
	}
	return bRet;
}
/*读取超时次数*/
bool ZinxTimerChannel::ReadFd(std::string& _input)
{
	bool bRet = false;
	char buf[8] = { 0 };
	
	if (sizeof(buf) == read(m_TimerFd, buf, sizeof(buf)))
	{
		bRet = true;
		//str1.assgin(char * buf, n);  将部分的前n个字节赋值给str1
		_input.assign(buf,sizeof(buf));
	}
	return bRet;
}

bool ZinxTimerChannel::WriteFd(std::string& _output)
{
	return false;
}
/*关闭定时器文件表*/
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

/*通过烤制标准输出通道去输出hello world
继承于事件类*/
class output_hello :public AZinxHandler {
	/*信息处理函数，开发者重写该函数实现信息的处理，
	当有需要一个环节继续处理时，
	应该创建新的信息对象（堆对象）并返回指针*/
	virtual IZinxMsg* InternelHandle(IZinxMsg& _oInput)
	{
		auto pchannel = ZinxKernel::Zinx_GetChannel_ByInfo("stdout");
		std::string output = "hello world";
		ZinxKernel::Zinx_SendOut(output, *pchannel);
		return nullptr;
	}
	virtual AZinxHandler* GetNextHandler(IZinxMsg& _oNextMsg)
	{
		return nullptr;
	}

}*pout_hello = new output_hello();

/*返回处理超时事件
框架类似epoll的框架，当产生信号时自动在内核执行返回对象内的函数*/
AZinxHandler* ZinxTimerChannel::GetInputNextStage(BytesMsg& _oInput)
{
	return  &TimerOutMng::GetInstance();
}

TimerOutMng TimerOutMng::single;
TimerOutMng::TimerOutMng()
{
	//创建10个齿
	for (int i = 0; i < 10; i++)
	{
		list<TimerOutProc*> tmp;
		m_timer_wheel.push_back(tmp);
	}
}

IZinxMsg* TimerOutMng::InternelHandle(IZinxMsg& _oInput)
{
	unsigned int iTimerOutCount = 0;
	GET_REF2DATA(BytesMsg, obytes, _oInput);
	obytes.szData.copy((char*) & iTimerOutCount, sizeof(iTimerOutCount), 0);

	while (iTimerOutCount-- > 0)
	{
		/*移动刻度*/
		cur_index++;
		cur_index %= 10;
		list<TimerOutProc*>m_cache;
		/*遍历当前刻度所有节点，指向处理函数或圈数-1*/
		for (auto itr = m_timer_wheel[cur_index].begin(); itr != m_timer_wheel[cur_index].end();)
		{
			if ((*itr)->iCount <= 0)
			{
				/*缓存待处理的超时节点*/
				m_cache.push_back(*itr);
				auto ptmp = *itr;
				//这里的itr返回的是链表下一个事件的迭代器，不是删除的，
				//所以这里要有个指针来记录删除事件的迭代器对应的指针，以便重新添加
				itr = m_timer_wheel[cur_index].erase(itr);
				AddTask(ptmp);
			}
			else
			{
				(*itr)->iCount--;
				itr++;
			}

		}
		/*统一处理待处理超时事件*/
		for (auto task : m_cache)
		{
			task->Proc();
		}
	}
	return nullptr;
}
AZinxHandler* TimerOutMng::GetNextHandler(IZinxMsg& _oNextMsg)
{
	return nullptr;
}
void TimerOutMng::AddTask(TimerOutProc* _ptask)
{
	/*计算当前任务需要放在哪个齿上*/
	int index = (_ptask->GetTimeSec()+cur_index) % 10;
	/*把任务放在该齿上*/
	m_timer_wheel[index].push_back(_ptask);
	/*计算所需圈数*/
	_ptask->iCount = _ptask->GetTimeSec()/10;
}
void TimerOutMng::DelTask(TimerOutProc* _ptask)
{
	/*遍历时间轮所有齿，删除掉任务*/
	for (auto &chi : m_timer_wheel)
	{
		for (auto task : chi)
		{
			if (task == _ptask)
			{
				chi.remove(_ptask);
				return ;
			}
		}
	}
}
