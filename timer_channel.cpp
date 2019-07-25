#include "timer_channel.h"
#include <sys/timerfd.h>
#include <stdio.h>

timer_channel::timer_channel()
{
}


timer_channel::~timer_channel()
{
}

/*����timerfd*/
bool timer_channel::Init()
{
	bool bRet = false;
	int fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (fd >= 0)
	{
		m_fd = fd;
		struct itimerspec period = { {1,0},{1,0} };
		//������ʱ��
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

/*_inputҪ����8�ֽڵĳ�ʱ����*/
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

/*���س�ʱ������Ķ���*/
AZinxHandler * timer_channel::GetInputNextStage(BytesMsg & _oInput)
{
	return timeout_deliver::GetInstance();
}

timeout_deliver::timeout_deliver()
{
	/*���δ���ʱ���ֵ����п̶�*/
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

//����_oInput�ǳ�ʱͨ��������ByteMsg  ����-��szData�ǳ�ʱ����
IZinxMsg * timeout_deliver::InternelHandle(IZinxMsg & _oInput)
{
	GET_REF2DATA(BytesMsg, timeout_count, _oInput);

	/*ǿת8���ֽڵ��׵�ַ�޷���������ָ��*/
	unsigned long *pcont = (unsigned long *)timeout_count.szData.data();

	for (int i = 0; i < *pcont; i++)
	{
		std::list<timeout_task *> tmp_list;
		/*������ǰ�̶�����Ӧ��list��������1*/
		for (auto itr = m_wheel[cur_pos].begin(); itr != m_wheel[cur_pos].end(); )
		{
			/*��������Ϊ0�ˣ���Ҫ���ô����������ü���*/
			if ((*itr)->m_count <= 0)
			{
				/*��ʱ�ˣ��ݴ汾�γ�ʱ������ڵ�*/
				tmp_list.push_back((*itr));
				/*Ҫժ��,����������*/
				itr = m_wheel[cur_pos].erase(itr);
				
			}
			else
			{
				/*������*/
				(*itr)->m_count--;
				itr++;
			}
		}
		/*���ó�ʱ����Ĵ�������������ӳ�ʱ����ʱ����*/
		for (auto ptask : tmp_list)
		{
			ptask->proc_timeout();
			register_task(ptask->time_out, ptask);
		}
		/*�̶�����ƶ�һ��*/
		cur_pos++;
		cur_pos %= m_wheel.size();
		/*ѭ����ʱ������*/
	}	

	/*û����һ������*/
	return nullptr;
}

AZinxHandler * timeout_deliver::GetNextHandler(IZinxMsg & _oNextMsg)
{
	/*û����һ������*/
	return nullptr;
}

void timeout_deliver::register_task(int _sec, timeout_task * _task)
{
	/*�����������ڸ���*/
	int dest_grid = _sec % m_wheel.size() + cur_pos;
	dest_grid %= m_wheel.size();
	/*����ʣ�����*/
	_task->m_count = _sec / m_wheel.size();
	_task->time_out = _sec;

	/*��Ӳ���_task���󵽶�Ӧ���ӵ�list��*/
	m_wheel[dest_grid].push_back(_task);
	
}

void timeout_deliver::unregister_task(timeout_task * _task)
{
	/*�������п̶�*/
	for (auto &grid : m_wheel)
	{
		bool find = false;
		/*�����ÿ̶ȶ�Ӧ��list---��ɾ��*/
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
