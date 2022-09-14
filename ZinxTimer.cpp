#include "ZinxTimer.h"
#include <sys/timerfd.h>
using namespace std;

ZinxTimerChannel::ZinxTimerChannel()
{}

ZinxTimerChannel::~ZinxTimerChannel()
{}
/*������ʱ���ļ���*/
bool ZinxTimerChannel::Init()
{
	bool bRet = false;
	/*�����ļ�������*/
	int iFd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (0 <= iFd)
	{
		/*���ö�ʱ����*/
		struct itimerspec period = { {1,0},{1,0} };
		if (0 == timerfd_settime(iFd, 0, &period, NULL))
		{
			bRet = true;
			m_TimerFd = iFd;
		}
	}
	return bRet;
}
/*��ȡ��ʱ����*/
bool ZinxTimerChannel::ReadFd(std::string& _input)
{
	bool bRet = false;
	char buf[8] = { 0 };
	
	if (sizeof(buf) == read(m_TimerFd, buf, sizeof(buf)))
	{
		bRet = true;
		//str1.assgin(char * buf, n);  �����ֵ�ǰn���ֽڸ�ֵ��str1
		_input.assign(buf,sizeof(buf));
	}
	return bRet;
}

bool ZinxTimerChannel::WriteFd(std::string& _output)
{
	return false;
}
/*�رն�ʱ���ļ���*/
void ZinxTimerChannel::Fini()
{
	close(m_TimerFd);
	m_TimerFd = -1;
}
/*���ص�ǰ�Ķ�ʱ���ļ�������*/
int ZinxTimerChannel::GetFd()
{
	return m_TimerFd;
}

std::string ZinxTimerChannel::GetChannelInfo()
{
	return "TimerFd";
}

/*ͨ�����Ʊ�׼���ͨ��ȥ���hello world
�̳����¼���*/
class output_hello :public AZinxHandler {
	/*��Ϣ����������������д�ú���ʵ����Ϣ�Ĵ���
	������Ҫһ�����ڼ�������ʱ��
	Ӧ�ô����µ���Ϣ���󣨶Ѷ��󣩲�����ָ��*/
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

/*���ش���ʱ�¼�
�������epoll�Ŀ�ܣ��������ź�ʱ�Զ����ں�ִ�з��ض����ڵĺ���*/
AZinxHandler* ZinxTimerChannel::GetInputNextStage(BytesMsg& _oInput)
{
	return  &TimerOutMng::GetInstance();
}

TimerOutMng TimerOutMng::single;
TimerOutMng::TimerOutMng()
{
	//����10����
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
		/*�ƶ��̶�*/
		cur_index++;
		cur_index %= 10;
		list<TimerOutProc*>m_cache;
		/*������ǰ�̶����нڵ㣬ָ��������Ȧ��-1*/
		for (auto itr = m_timer_wheel[cur_index].begin(); itr != m_timer_wheel[cur_index].end();)
		{
			if ((*itr)->iCount <= 0)
			{
				/*���������ĳ�ʱ�ڵ�*/
				m_cache.push_back(*itr);
				auto ptmp = *itr;
				//�����itr���ص���������һ���¼��ĵ�����������ɾ���ģ�
				//��������Ҫ�и�ָ������¼ɾ���¼��ĵ�������Ӧ��ָ�룬�Ա��������
				itr = m_timer_wheel[cur_index].erase(itr);
				AddTask(ptmp);
			}
			else
			{
				(*itr)->iCount--;
				itr++;
			}

		}
		/*ͳһ���������ʱ�¼�*/
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
	/*���㵱ǰ������Ҫ�����ĸ�����*/
	int index = (_ptask->GetTimeSec()+cur_index) % 10;
	/*��������ڸó���*/
	m_timer_wheel[index].push_back(_ptask);
	/*��������Ȧ��*/
	_ptask->iCount = _ptask->GetTimeSec()/10;
}
void TimerOutMng::DelTask(TimerOutProc* _ptask)
{
	/*����ʱ�������гݣ�ɾ��������*/
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
