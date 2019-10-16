#pragma once
#include <string>
#include <vector>
#include "libevent_cpp/include/include_all.h"
#include "log_def.h"
#include "svr_util/include/singleton.h"
#include "svr_util/include/easy_code.h"
#include "client_con.h"
#include "svr_con.h"

class Server: public Singleton<Server>
{
public:
	bool Init();
	ClientSvrCon *FindClientSvrCon(uint64 cid);

public:
	lc::Listener<SvrSvrCon> m_svr_listener;
	lc::Listener<ClientSvrCon> m_client_listener;
};

class CfgMgr : public Singleton<CfgMgr>
{
public:
	CfgMgr();
	bool Init();

	unsigned short GetInnerPort() const { return m_inner_port; }
	const char *GetInnerIp() const { return m_inner_ip.c_str(); }
	unsigned short GetExPort() const { return m_ex_port; }
	const char *GetExIp() const { return m_ex_ip.c_str(); }
	bool IsDaemon()const { return is_daemon; }
private:
	unsigned short m_inner_port;
	std::string m_inner_ip;
	//外网地址，给client连接用
	unsigned short m_ex_port; 
	std::string m_ex_ip;
	bool is_daemon;
};