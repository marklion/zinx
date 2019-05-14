#ifndef _ZINX_H_
#define _ZINX_H_
#include <string>
#include <list>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <fcntl.h>
#include <list>
#include <sys/wait.h>

#define GET_REF2DATA(type, ref, orig)  type * pref = dynamic_cast<type *>(&orig); if (nullptr == pref) {return nullptr;} type &ref = dynamic_cast<type&>(orig)

class IZinxMsg {
public:
	IZinxMsg() {}
	virtual ~IZinxMsg() {}
};

class AZinxHandler {
public:
	AZinxHandler() {}
	virtual ~AZinxHandler() {}
	void Handle(IZinxMsg &_oInput);
protected:
	virtual IZinxMsg *InternelHandle(IZinxMsg &_oInput) = 0;
	virtual AZinxHandler *GetNextHandler(IZinxMsg &_oNextMsg) = 0;
};

class SysIOReadyMsg :public IZinxMsg {
public:
	enum IO_DIC {
		IN, OUT
	} m_emIoDic;

	SysIOReadyMsg(IO_DIC _dic) :m_emIoDic(_dic) {}
};

class BytesMsg :public SysIOReadyMsg {
public:
	BytesMsg(SysIOReadyMsg &_base) :SysIOReadyMsg(_base.m_emIoDic) {}
	std::string szInfo;
	std::string szData;
};

class UserData {
public:
	UserData() {}
	virtual ~UserData() {}
};

class UserDataMsg :public BytesMsg {
public:
	UserDataMsg(BytesMsg &_base) :BytesMsg(_base) {}
	UserData *poUserData = NULL;
	virtual ~UserDataMsg()
	{
		if (NULL != poUserData)
		{
			delete poUserData;
		}
	}
};
class Irole :public AZinxHandler {
public:
	Irole() {}
	virtual ~Irole() {}
	virtual bool Init() = 0;
	virtual UserData *ProcMsg(UserData &_poUserData) = 0;
	virtual void Fini() = 0;
	void SetNextProcessor(Irole &_oNextRole)
	{
		poNextProcessor = &_oNextRole;
	}
	void ClearNextProcessor()
	{
		poNextProcessor = NULL;
	}
private:
	Irole *poNextProcessor = NULL;
	virtual IZinxMsg *InternelHandle(IZinxMsg &_oInput);
	virtual AZinxHandler *GetNextHandler(IZinxMsg &_oNextMsg);

};
class Ichannel;
class Iprotocol :public AZinxHandler {
public:
	Iprotocol() {}
	virtual ~Iprotocol() {}
	virtual UserData *raw2request(std::string _szInput) = 0;
	virtual std::string *response2raw(UserData &_oUserData) = 0;
protected:
	virtual Irole *GetMsgProcessor(UserDataMsg &_oUserDataMsg) = 0;
	virtual Ichannel *GetMsgSender(BytesMsg &_oBytes) = 0;
private:
	virtual IZinxMsg *InternelHandle(IZinxMsg &_oInput);
	virtual AZinxHandler *GetNextHandler(IZinxMsg &_oNextMsg);
};

class Ichannel :public AZinxHandler {
public:
	Ichannel() {};
	virtual ~Ichannel() {};
	virtual bool Init() = 0;
	virtual bool ReadFd(std::string &_input) = 0;
	virtual bool WriteFd(std::string &_output) = 0;
	virtual void Fini() = 0;
	virtual int GetFd() = 0;
	void FlushOut();
	bool HasOutput() { return false == m_WriteBuffer.empty(); }
	std::string Convert2Printable(std::string &_szData);
	void SetChannelClose() { m_NeedClose = true; }
	bool ChannelNeedClose() { return m_NeedClose; }
protected:
	virtual std::string GetChannelInfo() = 0;
	virtual AZinxHandler *GetInputNextStage(BytesMsg &_oInput) = 0;
private:
	virtual IZinxMsg *InternelHandle(IZinxMsg &_oInput);
	virtual AZinxHandler *GetNextHandler(IZinxMsg &_oNextMsg);
	std::list<std::string> m_WriteBuffer;
	bool m_NeedClose = false;
};

class ZinxKernel {
public:
	static bool ZinxKernelInit();
	static void ZinxKernelFini();
	static bool Zinx_Add_Channel(Ichannel &_oChannel);
	static void Zinx_Del_Channel(Ichannel &_oChannel);
	static void Zinx_SetChannelOut(Ichannel &_oChannel);
	static void Zinx_ClearChannelOut(Ichannel &_oChannel);
	static bool Zinx_Add_Proto(Iprotocol &_oProto);
	static void Zinx_Del_Proto(Iprotocol &_oProto);
	static bool Zinx_Add_Role(Irole &_oRole);
	static std::list<Irole *> &Zinx_GetAllRole();
	static void Zinx_Del_Role(Irole &_oRole);
	static void Zinx_Run();
	static void Zinx_SendOut(UserData &_oUserData, Iprotocol &_oProto);
	static void Zinx_SendOut(std::string &szBytes, Ichannel &_oChannel);
private:
	ZinxKernel();
	~ZinxKernel();
	bool Add_Channel(Ichannel &_oChannel);
	void Del_Channel(Ichannel &_oChannel);
	bool Add_Proto(Iprotocol &_oProto);
	void Del_Proto(Iprotocol &_oProto);
	bool Add_Role(Irole &_oRole);
	void Del_Role(Irole &_oRole);
	void Run();
	void SendOut(UserData &_oUserData);
	std::list<Ichannel *> m_ChannelList;
	std::list<Iprotocol *> m_ProtoList;
	std::list<Irole *> m_RoleList;
	int iEpollFd = -1;
	static ZinxKernel *poZinxKernel;
	bool m_need_exit = false;
public:
	static void Zinx_Exit()
	{
		poZinxKernel->m_need_exit = true;
	}
};

#endif

