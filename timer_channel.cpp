#include "timer_channel.h"
#include <sys/timerfd.h>
#include <stdio.h>

timer_channel::timer_channel()
{
}


timer_channel::~timer_channel()
{
}

/*创建timerfd*/
bool timer_channel::Init()
{
	bool bRet = false;
	int fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (fd >= 0)
	{
		m_fd = fd;
		struct itimerspec period = { {1,0},{1,0} };
		//开启定时器
		if (0 == timerfd_settime(m_fd, 0, &period, NULL))
		{
			bRet = true;
		}
		else
		{
			close(m_fd);
			m_fd = -1;
		}
		
	}
	return bRet;
}

/*_input要带出8字节的超时次数*/
bool timer_channel::ReadFd(std::string & _input)
{
	bool bRet = false;
	char buff[8];
	if (sizeof(buff) == read(m_fd, buff, sizeof(buff)))
	{
		_input.append(buff, sizeof(buff));
		bRet = true;
	}
	return bRet;
}

bool timer_channel::WriteFd(std::string & _output)
{
	return false;
}

void timer_channel::Fini()
{
	if (m_fd >= 0)
	{
		close(m_fd);
	}
}

int timer_channel::GetFd()
{
	return m_fd;
}

std::string timer_channel::GetChannelInfo()
{
	return "timerfd" + m_fd;
}

/*返回超时管理类的对象*/
AZinxHandler * timer_channel::GetInputNextStage(BytesMsg & _oInput)
{
	return timeout_deliver::GetInstance();
}

timeout_deliver::timeout_deliver()
{
	/*依次创建时间轮的所有刻度*/
	for (int i = 0; i < 10; i++)
	{
		std::list<timeout_task *> tmp;
		m_wheel.push_back(tmp);
	}
}

timeout_deliver::~timeout_deliver()
{
}

timeout_deliver *timeout_deliver::m_single = NULL;

timeout_deliver * timeout_deliver::GetInstance()
{
	if (m_single == NULL)
	{
		m_single = new timeout_deliver();
	}
	return m_single;
}

//参数_oInput是超时通道传来的ByteMsg  其中-》szData是超时次数
IZinxMsg * timeout_deliver::InternelHandle(IZinxMsg & _oInput)
{
	GET_REF2DATA(BytesMsg, timeout_count, _oInput);

	/*强转8个字节的首地址无符号整数的指针*/
	unsigned long *pcont = (unsigned long *)timeout_count.szData.data();

	for (int i = 0; i < *pcont; i++)
	{
		std::list<timeout_task *> tmp_list;
		/*遍历当前刻度所对应的list，减计数1*/
		for (auto itr = m_wheel[cur_pos].begin(); itr != m_wheel[cur_pos].end(); )
		{
			/*若计数减为0了，则要调用处理函数，重置计数*/
			if ((*itr)->m_count <= 0)
			{
				/*超时了，暂存本次超时的任务节点*/
				tmp_list.push_back((*itr));
				/*要摘掉,后边重新添加*/
				itr = m_wheel[cur_pos].erase(itr);
				
			}
			else
			{
				/*减计数*/
				(*itr)->m_count--;
				itr++;
			}
		}
		/*调用超时任务的处理函数，重新添加超时任务到时间轮*/
		for (auto ptask : tmp_list)
		{
			ptask->proc_timeout();
			register_task(ptask->time_out, ptask);
		}
		/*刻度向后移动一格*/
		cur_pos++;
		cur_pos %= m_wheel.size();
		/*循环超时次数次*/
	}	

	/*没有下一个环节*/
	return nullptr;
}

AZinxHandler * timeout_deliver::GetNextHandler(IZinxMsg & _oNextMsg)
{
	/*没有下一个环节*/
	return nullptr;
}

void timeout_deliver::register_task(int _sec, timeout_task * _task)
{
	/*计算任务所在格子*/
	int dest_grid = _sec % m_wheel.size() + cur_pos;
	dest_grid %= m_wheel.size();
	/*计算剩余计数*/
	_task->m_count = _sec / m_wheel.size();
	_task->time_out = _sec;

	/*添加参数_task对象到对应格子的list里*/
	m_wheel[dest_grid].push_back(_task);
	
}

void timeout_deliver::unregister_task(timeout_task * _task)
{
	/*遍历所有刻度*/
	for (auto &grid : m_wheel)
	{
		bool find = false;
		/*遍历该刻度对应的list---》删除*/
		for (auto ptask : grid)
		{
			if (ptask == _task)
			{
				grid.remove(_task);
				find = true;
				break;
			}
		}
		if (true == find)
		{
			break;
		}
	}
	
}
