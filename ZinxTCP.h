#ifndef _ZINXTCP_H_
#define _ZINXTCP_H_
#include "zinx.h"

class ZinxTcpData :public Ichannel {
private:
	int m_DataFd = -1;
public:
	ZinxTcpData(int _fd) :m_DataFd(_fd){}
	virtual ~ZinxTcpData() { close(m_DataFd); }

	virtual bool Init() override;
	virtual bool ReadFd(std::string & _input) override;
	virtual bool WriteFd(std::string & _output) override;
	virtual void Fini() override;
	virtual int GetFd() override;
	virtual std::string GetChannelInfo() override;
	virtual AZinxHandler * GetInputNextStage(BytesMsg & _oInput) = 0;
};

class IZinxTcpConnFact {
public:
	virtual ZinxTcpData *CreateTcpDataChannel(int _fd) = 0;
};

class ZinxTCPListen :
	public Ichannel
{
private:
	unsigned short m_usPort = 0;
	int m_fd = -1;
	IZinxTcpConnFact *m_ConnFac = NULL;
public:
	ZinxTCPListen(unsigned short _usPort, IZinxTcpConnFact *_pConnFac) :m_usPort(_usPort), m_ConnFac(_pConnFac){}
	virtual ~ZinxTCPListen();

	virtual bool Init() override;
	virtual bool ReadFd(std::string & _input) override;
	virtual bool WriteFd(std::string & _output) override;
	virtual void Fini() override;
	virtual int GetFd() override;
	virtual std::string GetChannelInfo() override;
	virtual AZinxHandler * GetInputNextStage(BytesMsg & _oInput);
};
#endif
