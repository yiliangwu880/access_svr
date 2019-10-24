#include "acc_driver.h"
#include "connect.h"
#include "log_def.h"

using namespace std;
using namespace lc;
using namespace su;
using namespace acc;



acc::AccClientCon::AccClientCon(AccFacadeMgr &facade, ConMgr &con_mgr)
	:m_facade(facade)
	, m_con_mgr(con_mgr)
{

}

void acc::AccClientCon::OnRecv(const lc::MsgPack &msg)
{
	ASMsg as_data;
	L_COND(as_data.Parse(msg.data, msg.len));
	switch (as_data.cmd)
	{
	case CMD_RSP_REG:
	{
		HandleRspReg(as_data);
	}
	return;
	default:
		L_ERROR("unknow cmd. %d", as_data.cmd);
		break;
	}

}

void acc::AccClientCon::OnConnected()
{
	MsgReqReg req;
	req.svr_id = m_con_mgr.GetSvrId();
	Send(CMD_REQ_REG, req);
}

void acc::AccClientCon::OnDisconnected()
{
	auto f = std::bind(&AccClientCon::OnTryReconTimeOut, this);
	m_recon_tm.StopTimer();
	m_recon_tm.StartTimer(RE_CON_INTERVAL_SEC, f);
}

void acc::AccClientCon::HandleRspReg(const ASMsg &msg)
{
	L_COND(!m_is_reg, "repeated rsp reg");

	MsgRspReg rsp;
	bool ret = CtrlMsgProto::Parse(msg, rsp);
	L_COND(ret, "parse ctrl msg fail");
	if (0 == rsp.svr_id)
	{
		L_FATAL("reg error, stop all connect to acc");
		m_con_mgr.SetFatal();
		return;
	}
	L_COND(rsp.svr_id == m_con_mgr.GetSvrId());

	m_is_reg = true;
	bool is_unreg = false; //true表示有连接未注册
	for (AccClientCon *p : m_con_mgr.GetAllCon())
	{
		L_COND(p);
		if (!p->IsConnect())
		{
			continue;
		}
		if (!p->m_is_reg)
		{
			is_unreg = true;
			break;
		}
	}
	if (!is_unreg)
	{
		m_con_mgr.SetRegSuccess(rsp.svr_id);
	}
}

void acc::AccClientCon::OnTryReconTimeOut()
{
	L_COND(!IsConnect());
	TryReconnect();
}

acc::SessionId::SessionId()
	:cid(0)
	,acc_id(0)
{

}

bool acc::SessionId::operator<(const SessionId &a) const
{
	if (cid == a.cid)
	{
		return acc_id < a.acc_id;
	}
	return cid < a.cid;
}

acc::Session::Session()
	:remote_port(0)
{

}



acc::ConMgr::ConMgr(AccFacadeMgr &facade)
	:m_facade(facade)
	, m_svr_id(0)
	, m_is_fatal(false)
{

}

acc::ConMgr::~ConMgr()
{
	FreeAllCon();
}

bool acc::ConMgr::Init(const std::vector<Addr> &vec_addr, uint16 svr_id)
{
	L_COND_F(!vec_addr.empty());
	L_COND_F(m_vec_con.empty(), "repeated init");
	{//check repeated
		std::set<Addr> s;
		for (auto &v : vec_addr)
		{
			if (!s.insert(v).second)
			{
				L_ERROR("repeated addr %s %d", v.ip.c_str(), v.port);
				return false;
			}
		}
	}

	m_svr_id = svr_id;
	//失败会内存泄露，不重复调用没事。
	for (auto &v : vec_addr)
	{
		AccClientCon *p = new AccClientCon(m_facade, *this);
		L_COND_F(p->ConnectInit(v.ip.c_str(), v.port));
		m_vec_con.push_back(p);
	}
	return true;
}

bool acc::ConMgr::AddAcc(const Addr &addr)
{
	for (AccClientCon *p : m_vec_con)
	{
		std::string ip;
		unsigned short port;
		p->GetRemoteAddr(ip, port);
		if (addr.ip == ip && addr.port == port)
		{
			L_ERROR("repeated connect addr")
				return false;
		}
	}
	AccClientCon *p = new AccClientCon(m_facade, *this);
	L_COND_F(p->ConnectInit(addr.ip.c_str(), addr.port));
	m_vec_con.push_back(p);
	return true;
}

void acc::ConMgr::SetRegSuccess(uint16 svr_id)
{
	m_facade.OnRegResult(svr_id);
}

void acc::ConMgr::SetFatal()
{
	m_is_fatal = true;
	FreeAllCon();
}

void acc::ConMgr::FreeAllCon()
{
	for (AccClientCon *v : m_vec_con)
	{
		delete v;
	}
	m_vec_con.clear();
}

bool acc::Addr::operator<(const Addr &other) const
{
	if (port == other.port)
	{
		return ip < other.ip;
	}
	return port < other.port;
}
