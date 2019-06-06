#include "zinx.h"
#include <sys/epoll.h>
#include <unistd.h>

ZinxKernel::ZinxKernel()
{
	iEpollFd = epoll_create(1);
}

ZinxKernel::~ZinxKernel()
{
	for (auto itr : m_ChannelList)
	{
		itr->Fini();
		delete itr;
	}
	if (iEpollFd >= 0)
	{
		close(iEpollFd);
	}
}

bool ZinxKernel::Add_Channel(Ichannel & _oChannel)
{
	bool bRet = false;

	if (true == _oChannel.Init())
	{
		struct epoll_event stEvent;
		stEvent.events = EPOLLIN|EPOLLOUT;
		stEvent.data.ptr = &_oChannel;

		if (0 == epoll_ctl(iEpollFd, EPOLL_CTL_ADD, _oChannel.GetFd(), &stEvent))
		{
			m_ChannelList.push_back(&_oChannel);
			bRet = true;
		}
	}

	return bRet;
}

void ZinxKernel::Del_Channel(Ichannel & _oChannel)
{
	m_ChannelList.remove(&_oChannel);
	epoll_ctl(iEpollFd, EPOLL_CTL_DEL, _oChannel.GetFd(), NULL);
	_oChannel.Fini();
}

bool ZinxKernel::Add_Proto(Iprotocol & _oProto)
{
	m_ProtoList.push_back(&_oProto);
	return true;
}

void ZinxKernel::Del_Proto(Iprotocol & _oProto)
{
	m_ProtoList.remove(&_oProto);
}

bool ZinxKernel::Add_Role(Irole & _oRole)
{
	bool bRet = false;

	if (true == _oRole.Init())
	{
		m_RoleList.push_back(&_oRole);
		bRet = true;
	}

	return bRet;
}

void ZinxKernel::Del_Role(Irole & _oRole)
{
	m_RoleList.remove(&_oRole);
	_oRole.Fini();
}

void ZinxKernel::Run()
{
	int iEpollRet = -1;

	while (false == m_need_exit)
	{
		struct epoll_event atmpEvent[100];
		iEpollRet = epoll_wait(iEpollFd, atmpEvent, 100, -1);
		if (-1 == iEpollRet)
		{
			if (EINTR == errno)
			{
				continue;
			}
			else
			{
				break;
			}
		}
		for (int i = 0; i < iEpollRet; i++)
		{
			Ichannel *poChannel = static_cast<Ichannel *>(atmpEvent[i].data.ptr);
			if (0 != (EPOLLIN & atmpEvent[i].events))
			{
				SysIOReadyMsg IoStat = SysIOReadyMsg(SysIOReadyMsg::IN);
				poChannel->Handle(IoStat);
				if (true == poChannel->ChannelNeedClose())
				{
					ZinxKernel::Del_Channel(*poChannel);
					delete poChannel;
					break;
				}
			}
			if (0 != (EPOLLOUT & atmpEvent[i].events))
			{
				poChannel->FlushOut();
				if (false == poChannel->HasOutput())
				{
					Zinx_ClearChannelOut(*poChannel);
				}
			}
		}
	}
}
void ZinxKernel::SendOut(UserData & _oUserData)
{

}
ZinxKernel *ZinxKernel::poZinxKernel = NULL;
bool ZinxKernel::ZinxKernelInit()
{
	if (NULL == poZinxKernel)
	{
		poZinxKernel = new ZinxKernel();
	}
	
	return true;
}

void ZinxKernel::ZinxKernelFini()
{
	delete poZinxKernel;
	poZinxKernel = NULL;
}

bool ZinxKernel::Zinx_Add_Channel(Ichannel & _oChannel)
{
	return poZinxKernel->Add_Channel(_oChannel);
}

void ZinxKernel::Zinx_Del_Channel(Ichannel & _oChannel)
{
	poZinxKernel->Del_Channel(_oChannel);
}

Ichannel *ZinxKernel::Zinx_GetChannel_ByInfo(std::string _szInfo)
{
    Ichannel *pret = NULL;

    for (auto itr:poZinxKernel->m_ChannelList)
    {
        if (_szInfo == (*itr).GetChannelInfo())
        {
            pret = &(*itr);
        }
    }

    return pret;
}

void ZinxKernel::Zinx_SetChannelOut(Ichannel & _oChannel)
{
	struct epoll_event stEvent;
	stEvent.events = EPOLLIN | EPOLLOUT;
	stEvent.data.ptr = &_oChannel;

	epoll_ctl(poZinxKernel->iEpollFd, EPOLL_CTL_MOD, _oChannel.GetFd(), &stEvent);
}

void ZinxKernel::Zinx_ClearChannelOut(Ichannel & _oChannel)
{
	struct epoll_event stEvent;
	stEvent.events = EPOLLIN;
	stEvent.data.ptr = &_oChannel;

	epoll_ctl(poZinxKernel->iEpollFd, EPOLL_CTL_MOD, _oChannel.GetFd(), &stEvent);
}

bool ZinxKernel::Zinx_Add_Proto(Iprotocol & _oProto)
{
	return poZinxKernel->Add_Proto(_oProto);
}

void ZinxKernel::Zinx_Del_Proto(Iprotocol & _oProto)
{
	poZinxKernel->Del_Proto(_oProto);
}

bool ZinxKernel::Zinx_Add_Role(Irole & _oRole)
{
	return poZinxKernel->Add_Role(_oRole);
}

std::list<Irole*> &ZinxKernel::Zinx_GetAllRole()
{
	return poZinxKernel->m_RoleList;
}

void ZinxKernel::Zinx_Del_Role(Irole & _oRole)
{
	poZinxKernel->Del_Role(_oRole);
}

void ZinxKernel::Zinx_Run()
{
	poZinxKernel->Run();
}

void ZinxKernel::Zinx_SendOut(UserData & _oUserData, Iprotocol & _oProto)
{
	SysIOReadyMsg iodic = SysIOReadyMsg(SysIOReadyMsg::OUT);
	BytesMsg oBytes = BytesMsg(iodic);
	UserDataMsg oUserDataMsg = UserDataMsg(oBytes);

	oUserDataMsg.poUserData = &_oUserData;
	_oProto.Handle(oUserDataMsg);
}

void ZinxKernel::Zinx_SendOut(std::string & szBytes, Ichannel & _oChannel)
{
	SysIOReadyMsg iodic = SysIOReadyMsg(SysIOReadyMsg::OUT);
	BytesMsg oBytes = BytesMsg(iodic);
	oBytes.szData = szBytes;
	_oChannel.Handle(oBytes);
}

