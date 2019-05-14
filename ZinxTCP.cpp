#include "ZinxTCP.h"

ZinxTCPListen::~ZinxTCPListen()
{
	delete m_ConnFac;
}

bool ZinxTCPListen::Init()
{
	bool bRet = false;
	int iListenFd = -1;
	struct sockaddr_in stServaddr;

	iListenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (0 <= iListenFd)
	{
		memset(&stServaddr, 0, sizeof(stServaddr));
		stServaddr.sin_family = AF_INET;
		stServaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		stServaddr.sin_port = htons(m_usPort);

		int opt = 1;
		setsockopt(iListenFd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt));

		if (0 == bind(iListenFd, (struct sockaddr *)&stServaddr, sizeof(stServaddr)))
		{
			if (0 == listen(iListenFd, 10))
			{
				bRet = true;
				m_fd = iListenFd;
			}
			else
			{
				perror("__FUNC__:listen:");
			}
		}
		else
		{
			perror("__FUNC__:bind:");
		}
	}
	else
	{
		perror("__FUNC__:socket:");
	}


	return bRet;
}

bool ZinxTCPListen::ReadFd(std::string & _input)
{
	bool bRet = false;

	int iDataFd = -1;
	struct sockaddr_in stClientAddr;
	socklen_t lAddrLen = sizeof(stClientAddr);

	iDataFd = accept(m_fd, (struct sockaddr *)&stClientAddr, &lAddrLen);
	if (0 <= iDataFd)
	{
		auto poTcpDataChannel = m_ConnFac->CreateTcpDataChannel(iDataFd);
		if (NULL != poTcpDataChannel)
		{
			bRet = ZinxKernel::Zinx_Add_Channel(*poTcpDataChannel);
		}
	}
	else
	{
		perror("__FUNC__:accept:");
	}
	
	return bRet;
}

bool ZinxTCPListen::WriteFd(std::string & _output)
{
	return false;
}

void ZinxTCPListen::Fini()
{
	if (0 <= m_fd)
	{
		close(m_fd);
		m_fd = -1;
	}
}

int ZinxTCPListen::GetFd()
{
	return m_fd;
}

std::string ZinxTCPListen::GetChannelInfo()
{
	return "TcpListen";
}

AZinxHandler * ZinxTCPListen::GetInputNextStage(BytesMsg & _oInput)
{
	return nullptr;
}

bool ZinxTcpData::Init()
{
	return true;
}

bool ZinxTcpData::ReadFd(std::string & _input)
{
	bool bRet = false;
	ssize_t iReadLen = -1;
	char acBuff[1024] = { 0 };

	while (0 < (iReadLen = recv(m_DataFd, acBuff, sizeof(acBuff), MSG_DONTWAIT)))
	{
		bRet = true;
		_input.append(acBuff, iReadLen);
	}

	std::cout << "<----------------------------------------->" << std::endl;
	std::cout << "recv from " << m_DataFd << ":" << Ichannel::Convert2Printable(_input) << std::endl;
	std::cout << "<----------------------------------------->" << std::endl;

	if (false == bRet)
	{
		SetChannelClose();
	}

	return bRet;

}

bool ZinxTcpData::WriteFd(std::string & _output)
{
	bool bRet = false;
	char *pOut = (char *)calloc(1UL, _output.size());
	_output.copy(pOut, _output.size(), 0);
	if ((0 <= m_DataFd) && (_output.size() == send(m_DataFd,pOut,_output.size(), 0)))
	{
		bRet = true;
		std::cout << "<----------------------------------------->" << std::endl;
		std::cout << "send to " << m_DataFd << ":" << Ichannel::Convert2Printable(_output) << std::endl;
		std::cout << "<----------------------------------------->" << std::endl;
	}
	free(pOut);
	return bRet;
}

void ZinxTcpData::Fini()
{
	close(m_DataFd);
	m_DataFd = -1;
}

int ZinxTcpData::GetFd()
{
	return m_DataFd;
}

std::string ZinxTcpData::GetChannelInfo()
{
	return "TcpConnOn" + m_DataFd;
}
