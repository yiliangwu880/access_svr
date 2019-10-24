#include "acc_driver.h"
#include "connect.h"
#include "log_def.h"

using namespace std;
using namespace lc;
using namespace su;
using namespace acc;



acc::AccClientCon::AccClientCon(AccFacadeMgr &facade, ConMgr &con_mgr, uint32 acc_id)
	:m_acc_id(acc_id)
	, m_is_reg(false)
	,m_facade(facade)
	, m_con_mgr(con_mgr)
{

}

void acc::AccClientCon::OnRecv(const lc::MsgPack &msg)
{
	ASMsg as_data;
	L_COND(as_data.Parse(msg.data, msg.len));

	if (CMD_RSP_REG != as_data.cmd)
	{
		L_COND(m_is_reg);//svr处理非CMD_RSP_REG消息,必须已注册。
	}

	switch (as_data.cmd)
	{
	case CMD_RSP_REG           :HandleRspReg(as_data); return;
	case CMD_NTF_CREATE_SESSION:HandleCreateSession(as_data); return;
	case CMD_NTF_VERIFY_REQ:HandleVerifyReq(as_data); return;
	case CMD_NTF_FORWARD:HandleMsgForward(as_data); return;
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
	m_is_reg = false;
	auto f = std::bind(&AccClientCon::OnTryReconTimeOut, this);
	m_recon_tm.StopTimer();
	m_recon_tm.StartTimer(RE_CON_INTERVAL_SEC, f);
}

const acc::Session * acc::AccClientCon::FindSession(const SessionId &id)
{
	auto it = m_id_2_s.find(id);
	if (it == m_id_2_s.end())
	{
		return nullptr;
	}
	return &(it->second);
}

bool acc::AccClientCon::Send(const acc::ASMsg &as_data)
{
	std::string tcp_pack;
	L_COND_F(as_data.Serialize(tcp_pack));
	return SendPack(tcp_pack.c_str(), tcp_pack.length());
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
		m_con_mgr.SetRegSuccess(0);
		return;
	}
	L_COND(rsp.svr_id == m_con_mgr.GetSvrId());//可能请求代码ID出错

	m_is_reg = true;
	m_con_mgr.SetRegSuccess(rsp.svr_id);
}

void acc::AccClientCon::HandleCreateSession(const ASMsg &msg)
{
	MsgNtfCreateSession rsp;
	bool ret = CtrlMsgProto::Parse(msg, rsp);
	L_COND(ret, "parse ctrl msg fail");

	Session s;
	s.id.cid = rsp.cid;
	s.id.acc_id = m_acc_id;
	bool r = m_id_2_s.insert(make_pair(s.id, s)).second;
	if (!r)
	{
		L_ERROR("create session fail, repeated insert id. cid=%lld acc_id=%d", s.id.cid, s.id.acc_id);
		return;
	}
	return;
}

void acc::AccClientCon::HandleMsgForward(const ASMsg &msg)
{
	MsgForward f_msg;
	bool ret = f_msg.Parse(msg.msg, msg.msg_len);
	L_COND(ret, "parse ctrl msg fail");

	SessionId id;
	id.cid = f_msg.cid;
	id.acc_id = m_acc_id;
	auto it = m_id_2_s.find(id);
	if (it == m_id_2_s.end())
	{
		L_ERROR("can't find session when rev forward msg. %lld %d", id.cid, id.acc_id);
		return;
	}
	m_facade.OnRevClientMsg(id, f_msg.cmd, f_msg.msg, f_msg.msg_len);
}

void acc::AccClientCon::HandleVerifyReq(const ASMsg &msg)
{
	MsgForward f_msg;
	bool ret = f_msg.Parse(msg.msg, msg.msg_len);
	L_COND(ret, "parse ctrl msg fail");

	SessionId tmp_id; //临时会话id
	tmp_id.cid = f_msg.cid;
	tmp_id.acc_id = m_acc_id;
	m_facade.OnRevVerifyReq(tmp_id, f_msg.cmd, f_msg.msg, f_msg.msg_len);
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
		AccClientCon *p = new AccClientCon(m_facade, *this, m_vec_con.size());
		L_COND_F(p->ConnectInit(v.ip.c_str(), v.port));
		m_vec_con.push_back(p);
	}
	return true;
}

bool acc::ConMgr::AddAcc(const Addr &addr)
{
	L_COND_F(0 != m_svr_id, "un init ConMgr, can't cal AddAcc"); 
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
	AccClientCon *p = new AccClientCon(m_facade, *this, m_vec_con.size());
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

acc::AccClientCon * acc::ConMgr::FindAccClientCon(const SessionId &id) const
{
	L_COND_R(id.acc_id >= m_vec_con.size(), nullptr, "acc_id overload %d", id.acc_id);

	return m_vec_con[id.acc_id];
}

const acc::Session * acc::ConMgr::FindSession(const SessionId &id) const
{
	AccClientCon *con = FindAccClientCon(id);
	if (nullptr == con)
	{
		return nullptr;
	}
	return con->FindSession(id);
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
