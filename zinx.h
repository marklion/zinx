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

/*定义动态类型转换后的引用类型，若转换失败则执行返回NULL*/
#define GET_REF2DATA(type, ref, orig)  type * pref = dynamic_cast<type *>(&orig); if (nullptr == pref) {return nullptr;} type &ref = dynamic_cast<type&>(orig)

/*所有handler实例之间传递的消息父类*/
class IZinxMsg {
public:
	IZinxMsg() {}
	virtual ~IZinxMsg() {}
};

/*处理这handler抽象类，需要重写处理信息方法和获取下一个处理者方法*/
class AZinxHandler {
public:
	AZinxHandler() {}
	virtual ~AZinxHandler() {}
	void Handle(IZinxMsg &_oInput);
protected:
    /*信息处理函数，开发者重写该函数实现信息的处理，当有需要一个环节继续处理时，应该创建新的信息对象（堆对象）并返回指针*/
	virtual IZinxMsg *InternelHandle(IZinxMsg &_oInput) = 0;

	/*获取下一个处理环节函数，开发者重写该函数，可以指定当前处理环节结束后下一个环节是什么，通过参数信息对象，可以针对不同情况进行分别处理*/
	virtual AZinxHandler *GetNextHandler(IZinxMsg &_oNextMsg) = 0;
};

/*系统信息类，只包含当前信息的方向*/
class SysIOReadyMsg :public IZinxMsg {
public:
	enum IO_DIC {
		IN, OUT
	} m_emIoDic;

	SysIOReadyMsg(IO_DIC _dic) :m_emIoDic(_dic) {}
};

/*字节流信息类，包含string封装的字节流，string封装的通道信息和父类中的方向*/
class BytesMsg :public SysIOReadyMsg {
public:
	BytesMsg(SysIOReadyMsg &_base) :SysIOReadyMsg(_base.m_emIoDic) {}
	std::string szInfo;
	std::string szData;
};

/*纯用户数据信息，需要开发者继承后添加业务相关字段*/
class UserData {
public:
	UserData() {}
	virtual ~UserData() {}
};

/*用户信息类（对纯用户数据类的封装），继承自字节流类，拥有一个纯用户数据对象*/
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

/*角色类，派生自基础处理者类，提供初始化，去初始化，处理信息，设置、清除下一个处理者的虚函数。
角色类是用于处理业务数据的，开发者应该将一个纯业务流程由一个或多个角色类的子类处理
角色对象应该被添加到zinxkernel中*/
class Irole :public AZinxHandler {
public:
	Irole() {}
	virtual ~Irole() {}
	
	/*初始化函数，开发者可以重写该方法实现对象相关的初始化，该函数会在role对象添加到zinxkernel时调用*/
	virtual bool Init() = 0;

	/*处理信息函数，重写该方法可以对业务数据进行处理，若还需要后续处理流程，则应返回相关数据信息（堆对象）*/
	virtual UserData *ProcMsg(UserData &_poUserData) = 0;

	/*去初始化函数，类似初始化函数，该函数会在role对象摘除出zinxkernel时调用*/
	virtual void Fini() = 0;

	/*设置下一个处理者函数，开发者可以调用该函数，在运行时对流程进行拆分嫁接*/
	void SetNextProcessor(Irole &_oNextRole)
	{
		poNextProcessor = &_oNextRole;
	}
	/*清除下一个处理者函数，开发者可以调用该函数，在运行时对流程进行拆分嫁接*/
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

/*协议类，派生自基础处理者类，提供基于通信协议的，原始数据和业务数据之间的转换
一般地，协议对象会被指定为通道对象的下一个处理环节*/
class Iprotocol :public AZinxHandler {
public:
	Iprotocol() {}
	virtual ~Iprotocol() {}
	/*原始数据和业务数据相互函数，开发者重写该函数，实现协议*/
	virtual UserData *raw2request(std::string _szInput) = 0;
	
	/*原始数据和业务数据相互函数，开发者重写该函数，实现协议*/
	virtual std::string *response2raw(UserData &_oUserData) = 0;
protected:
    /*获取处理角色对象函数，开发者应该重写该函数，用来指定当前产生的用户数据消息应该传递给哪个角色处理*/
	virtual Irole *GetMsgProcessor(UserDataMsg &_oUserDataMsg) = 0;

	/*获取发送通道函数，开发者应该重写该函数，用来指定当前字节流应该由哪个通道对象发出*/
	virtual Ichannel *GetMsgSender(BytesMsg &_oBytes) = 0;
private:
	virtual IZinxMsg *InternelHandle(IZinxMsg &_oInput);
	virtual AZinxHandler *GetNextHandler(IZinxMsg &_oNextMsg);
};

/*通道类，派生自基础处理者类，提供基于系统调用的数据收发功能
一般地，用户应该根据处理的文件（信息源）不同而创建通道类的子类或选用合适的实用类（已经提供的通道类子类）来完成系统级文件IO*/
class Ichannel :public AZinxHandler {
public:
	Ichannel() {};
	virtual ~Ichannel() {};
	
	/*通道初始化函数，一般地，开发者应该重写这个函数实现打开文件和一定的参数配置
	该函数会在通道对象添加到zinxkernel时被调用*/
	virtual bool Init() = 0;

	/*读取数据， 开发者应该根据目标文件不同，重写这个函数，以实现将fd中的数据读取到参数_string中
	该函数会在数据源所对应的文件有数据到达时被调用*/
	virtual bool ReadFd(std::string &_input) = 0;

	/*写出数据， 开发者应该将数据写出的操作通过重写该函数实现
	该函数会在调用zinxkernel::sendout函数后被调用（通常不是直接调用而是通过多路复用的反压机制调用）*/
	virtual bool WriteFd(std::string &_output) = 0;

	/*通道去初始化，开发者应该在该函数回收相关资源
	该函数会在通道对象从zinxkernel中摘除时调用*/
	virtual void Fini() = 0;

	/*获取文件描述符函数， 开发者应该在该函数返回当前关心的文件，
	一般地，开发者应该在派生类中定义属性用来记录数据来记录当前IO所用的文件，在此函数中只是返回该属性*/
	virtual int GetFd() = 0;
	void FlushOut();
	bool HasOutput() { return false == m_WriteBuffer.empty(); }
	std::string Convert2Printable(std::string &_szData);

	/*设置通道关闭函数，若再数据处理或业务处理时，判断当前通道应该被关闭时，可以调用该函数*/
	void SetChannelClose() { m_NeedClose = true; }
	bool ChannelNeedClose() { return m_NeedClose; }
    /*获取通道信息函数，开发者可以在该函数中返回跟通道相关的一些特征字符串，方便后续查找和过滤*/
	virtual std::string GetChannelInfo() = 0;
protected:
	/*获取下一个处理环节，开发者应该重写该函数，指定下一个处理环节
	一般地，开发者应该在该函数返回一个协议对象，用来处理读取到的字节流*/
	virtual AZinxHandler *GetInputNextStage(BytesMsg &_oInput) = 0;
private:
	virtual IZinxMsg *InternelHandle(IZinxMsg &_oInput);
	virtual AZinxHandler *GetNextHandler(IZinxMsg &_oNextMsg);
	std::list<std::string> m_WriteBuffer;
	bool m_NeedClose = false;
};

/*zinx并发核心，整个框架既是一个单例，提供若干静态方法进行操作。*/
class ZinxKernel {
	static void Zinx_SetChannelOut(Ichannel &_oChannel);
	static void Zinx_ClearChannelOut(Ichannel &_oChannel);
	friend class Ichannel;
public:
    /*初始化，每个要用到zinx框架的进程应该调用且只调用一次该函数*/
	static bool ZinxKernelInit();
	
	/*去初始化，在进程退出前建议调用该函数，回收相关资源*/
	static void ZinxKernelFini();

	/*添加通道对象，要求对象为堆对象，通道对象只有添加到zinxkernel后才能被并发处理*/
	static bool Zinx_Add_Channel(Ichannel &_oChannel);

	/*摘除通道对象，该函数不会释放通道对象，需要调用者手动释放*/
	static void Zinx_Del_Channel(Ichannel &_oChannel);

    /*通过通道的信息获取通道对象，如果多个通道的信息字符串相同则获取第一个通道*/
	static Ichannel *Zinx_GetChannel_ByInfo(std::string _szInfo);

    /*添加协议对象，要求对象为堆对象，该函数仅有内存管理的作用，建议开发者将协议对象添加到zinxkernel存储*/
	static bool Zinx_Add_Proto(Iprotocol &_oProto);

	/*摘除协议对象，该函数不会释放协议对象，需要调用者手动释放*/
	static void Zinx_Del_Proto(Iprotocol &_oProto);

	/*添加角色对象，要求对象为堆对象，添加到zinxkernel后的角色对象才能被调用初始化函数*/
	static bool Zinx_Add_Role(Irole &_oRole);

	/*获取角色对象列表，获取当前zinxkernel中所有的角色对象*/
	static std::list<Irole *> &Zinx_GetAllRole();

	/*摘除角色对象，该函数不会释放角色对象，需要调用者手动释放*/
	static void Zinx_Del_Role(Irole &_oRole);

	/*运行框架，该函数运行后会一直循环处理IO数据，直到Zinx_Exit被调用*/
	static void Zinx_Run();

	/*向外发送数据，将参数1指定的用户数据通过参数2指定的协议对象发出*/
	static void Zinx_SendOut(UserData &_oUserData, Iprotocol &_oProto);

	/*向外发送数据，将参数1指定的字节流通过参数2指定的通道发出*/
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
    /*停止框架*/
	static void Zinx_Exit()
	{
		poZinxKernel->m_need_exit = true;
	}
};

#endif

