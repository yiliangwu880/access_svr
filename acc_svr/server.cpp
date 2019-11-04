#include "server.h"
#include "svr_util/include/read_cfg.h"

using namespace std;
using namespace acc;
using namespace lc;

bool Server::Init()
{
	bool r = m_svr_listener.Init(CfgMgr::Obj().GetInnerPort(), CfgMgr::Obj().GetInnerIp());
	L_COND_F(r);

	r = m_client_listener.Init(CfgMgr::Obj().GetExPort(), CfgMgr::Obj().GetExIp());
	L_COND_F(r);

	L_DEBUG("Server::Init ok");
	return true;
}

ExternalSvrCon * Server::FindClientSvrCon(uint64 cid)
{
	BaseConMgr &conMgr = m_client_listener.GetConnMgr();
	SvrCon *pClient = conMgr.FindConn(cid);
	ExternalSvrCon *p = dynamic_cast<ExternalSvrCon *>(pClient);
	if (nullptr == p)
	{
		L_DEBUG("find ClientSvrCon fail. cid=%lld", cid);
	}
	return p;
}

uint32 Server::GetExConSize() const
{
	return m_client_listener.GetConstConnMgr().GetConSize();
}

CfgMgr::CfgMgr()
	:m_inner_port(0)
	, m_ex_port(0)
	, is_daemon(false)
{

}

bool CfgMgr::Init()
{
	L_DEBUG("init cfg");
	su::Config cfg;
	L_COND_F(cfg.init("cfg.txt"));

	m_inner_ip = cfg.GetStr("inner_ip");
	L_DEBUG("inner_ip=%s", m_inner_ip.c_str());
	m_inner_port = (uint16)cfg.GetInt("inner_port");
	L_DEBUG("inner_port=%d", m_inner_port);

	m_ex_ip = cfg.GetStr("ex_ip");
	L_DEBUG("ex_ip=%s", m_ex_ip.c_str());
	m_ex_port = (uint16)cfg.GetInt("ex_port");
	L_DEBUG("ex_port=%d", m_ex_port);

	is_daemon = (bool)cfg.GetInt("is_daemon");
	L_DEBUG("is_daemon=%d", is_daemon);

	return true;
}
