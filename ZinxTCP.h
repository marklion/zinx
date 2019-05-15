#ifndef _ZINXTCP_H_
#define _ZINXTCP_H_
#include "zinx.h"

/*tcp数据套接字通道类，继承通道类，该类也是一个抽象类，需要开发者继承该类，
重写GetInputNextStage函数以指定读取到的字节流的处理方式*/
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

/*产生tcp数据套接字通道类的抽象工厂类，
开发者需要重写CreateTcpDataChannel函数，来返回一个tcp通道对象
一般地，开发者应该同时创建一对tcp通道类和工厂类*/
class IZinxTcpConnFact {
public:
	virtual ZinxTcpData *CreateTcpDataChannel(int _fd) = 0;
};

/*tcp监听通道类，这是一个实体类（不建议继承该类），开发者可以直接创建tcp监听通道对象，
一般地，开发者应该在该类的构造函数中，指定一个tcp套接字通道类的工厂类，当有连接到来后，该工厂类的成员方法会被调用*/
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
