#include "acc_driver.h"
#include "connect.h"
#include "log_def.h"

using namespace std;
using namespace lc;
using namespace su;
using namespace acc;

acc::ADClientCon::ADClientCon(ADFacadeMgr &facade, ConMgr &con_mgr, uint32 acc_id)
	:m_acc_id(acc_id)
	, m_is_reg(false)
	, m_facade(facade)
	, m_con_mgr(con_mgr)
{

}

void ADClientCon::OnRecv(const lc::MsgPack &msg)
{
	ASMsg as_data;
	L_COND(as_data.Parse(msg.data, msg.len));

	if (CMD_RSP_REG != as_data.cmd)
	{
		L_COND(m_is_reg, "must reg ok before rev other cmd %d", as_data.cmd);//svr处理非CMD_RSP_REG消息,必须已注册。
	}

	switch (as_data.cmd)
	{
	case CMD_RSP_REG               :HandleRspReg(as_data)              ; return;
	case CMD_NTF_CREATE_SESSION    :HandleCreateSession(as_data)       ; return;
	case CMD_NTF_VERIFY_REQ        :HandleVerifyReq(as_data)           ; return;
	case CMD_NTF_FORWARD           :HandleMsgForward(as_data)          ; return;
	case CMD_NTF_DISCON            :HandleMsgNtfDiscon(as_data)        ; return;
	case CMD_RSP_SET_MAIN_CMD_2_SVR:HandleMsgRspSetMainCmd2Svr(as_data); return;
	default:
		L_ERROR("unknow cmd. %d", as_data.cmd);
		break;
	}
}

void acc::ADClientCon::OnConnected()
{
	{
		MsgReqReg req;
		req.svr_id = m_con_mgr.GetSvrId();
		req.is_verify = m_con_mgr.IsVerify();
		L_COND(req.svr_id);
		Send(CMD_REQ_REG, req);
	}

	{
		const MsgReqSetHeartbeatInfo &req = m_con_mgr.GetHeartbeatInfo();
		if (req.cmd != 0)
		{
			L_DEBUG("OnConnected. send heartbeat info. %d %d %d", req.cmd, req.rsp_cmd, req.interval_sec);
			Send(CMD_REQ_SET_HEARTBEAT_INFO, req);
		}
	}
}

void acc::ADClientCon::OnDisconnected()
{
	L_DEBUG("start try recon timer , sec=%d", RE_CON_INTERVAL_SEC);
	m_is_reg = false;
	auto f = std::bind(&ADClientCon::OnTryReconTimeOut, this);
	m_recon_tm.StopTimer();
	m_recon_tm.StartTimer(RE_CON_INTERVAL_SEC*1000, f);
	string ip;
	uint16 port;
	GetRemoteAddr(ip, port);
	m_facade.OnAccDisCon(ip, port);
}

const acc::Session *acc::ADClientCon::FindSession(const SessionId &id)
{
	auto it = m_id_2_s.find(id.cid);
	if (it == m_id_2_s.end())
	{
		return nullptr;
	}
	return &(it->second);
}

bool acc::ADClientCon::Send(const acc::ASMsg &as_data)
{
	std::string tcp_pack;
	L_COND_F(as_data.Serialize(tcp_pack));
	return SendPack(tcp_pack);
}

void acc::ADClientCon::HandleRspReg(const ASMsg &msg)
{
	L_COND(!m_is_reg, "repeated rsp reg");

	MsgRspReg rsp;
	bool ret = CtrlMsgProto::Parse(msg, rsp);
	L_COND(ret, "parse ctrl msg fail");
	if (0 == rsp.svr_id)
	{
		L_FATAL("reg error, stop all connect to acc");
		m_con_mgr.SetRegResult(0);
		m_con_mgr.SetFatal();
		return;
	}
	L_COND(rsp.svr_id == m_con_mgr.GetSvrId());//可能请求代码ID出错

	m_is_reg = true;
	m_con_mgr.SetRegResult(true);
}

void acc::ADClientCon::HandleCreateSession(const ASMsg &msg)
{
	MsgNtfCreateSession rsp;
	bool ret = CtrlMsgProto::Parse(msg, rsp);
	L_COND(ret, "parse ctrl msg fail");
	L_COND(rsp.cid);

	Session s;
	s.id.cid = rsp.cid;
	s.id.acc_id = m_acc_id;
	bool r = m_id_2_s.insert(make_pair(s.id.cid, s)).second;
	if (!r)
	{
		L_ERROR("create session fail, repeated insert id. cid=%lld acc_id=%d", s.id.cid, s.id.acc_id);
		return;
	}
	m_facade.OnClientConnect(s.id);
	return;
}

void acc::ADClientCon::HandleMsgForward(const ASMsg &msg)
{
	MsgForward f_msg;
	bool ret = f_msg.Parse(msg.msg, msg.msg_len);
	L_COND(ret, "parse ctrl msg fail");

	SessionId id;
	id.cid = f_msg.cid;
	id.acc_id = m_acc_id;
	auto it = m_id_2_s.find(id.cid);
	if (it == m_id_2_s.end())
	{
		L_ERROR("can't find session when rev forward msg. %lld %d", id.cid, id.acc_id);
		return;
	}
	m_facade.OnRevClientMsg(id, f_msg.cmd, f_msg.msg, f_msg.msg_len);
}

void acc::ADClientCon::HandleVerifyReq(const ASMsg &msg)
{
	MsgForward f_msg;
	bool ret = f_msg.Parse(msg.msg, msg.msg_len);
	L_COND(ret, "parse ctrl msg fail");

	SessionId tmp_id; //临时会话id
	tmp_id.cid = f_msg.cid;
	tmp_id.acc_id = m_acc_id;
	m_facade.OnRevVerifyReq(tmp_id, f_msg.cmd, f_msg.msg, f_msg.msg_len);
}

void acc::ADClientCon::HandleMsgNtfDiscon(const ASMsg &msg)
{
	MsgNtfDiscon ntf;
	bool ret = CtrlMsgProto::Parse(msg, ntf);
	L_COND(ret, "parse ctrl msg fail");
	L_COND(ntf.cid);

	SessionId id;
	id.cid = ntf.cid;
	id.acc_id = m_acc_id;

	m_id_2_s.erase(id.cid);
	m_facade.OnClientDisCon(id);
}

void acc::ADClientCon::HandleMsgRspSetMainCmd2Svr(const ASMsg &msg)
{
	MsgRspSetMainCmd2Svr rsp;
	bool ret = CtrlMsgProto::Parse(msg, rsp);
	L_COND(ret, "parse ctrl msg fail");
	L_COND(rsp.cid);
	SessionId id;
	id.cid = rsp.cid;
	id.acc_id = m_acc_id;

	m_facade.OnSetMainCmd2SvrRsp(id, rsp.main_cmd, rsp.svr_id);
}

void acc::ADClientCon::OnTryReconTimeOut()
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

acc::ConMgr::ConMgr(ADFacadeMgr &facade)
	:m_facade(facade)
	, m_svr_id(0)
	, m_is_fatal(false)
	, m_is_reg(false)
	, m_is_verify_svr(false)
{

}

acc::ConMgr::~ConMgr()
{
	FreeAllCon();
}



bool acc::ConMgr::Init(const std::vector<Addr> &vec_addr, uint16 svr_id, bool is_verify_svr)
{
	L_COND_F(!m_is_fatal);
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
	m_is_verify_svr = is_verify_svr;
	for (auto &v : vec_addr)
	{
		ADClientCon *p = new ADClientCon(m_facade, *this, m_vec_con.size());
		if (!p->ConnectInit(v.ip.c_str(), v.port))
		{
			delete p;
			p = nullptr;
			L_ERROR("connect acc fail. ip port= %s, %d", v.ip.c_str(), v.port);
			return false;
		}
		m_vec_con.push_back(p);
	}
//	L_DEBUG("m_vec_con or acc size=%d", m_vec_con.size());
	return true;
}

bool acc::ConMgr::AddAcc(const Addr &addr, bool is_verify_svr)
{
	L_COND_F(!m_is_fatal);
	L_COND_F(0 != m_svr_id, "must call  init before call AddAcc"); 
	for (ADClientCon *p : m_vec_con)
	{
		std::string ip;
		unsigned short port;
		p->GetRemoteAddr(ip, port);
		if (addr.ip == ip && addr.port == port)
		{
			L_ERROR("repeated connect addr");
			return false;
		}
	}

	ADClientCon *p = new ADClientCon(m_facade, *this, m_vec_con.size());
	if(!p->ConnectInit(addr.ip.c_str(), addr.port))
	{
		delete p;
		p = nullptr;
		L_ERROR("connect acc fail. ip port= %s, %d", addr.ip.c_str(), addr.port);
		return false;
	}
	m_vec_con.push_back(p);
	return true;
}

void acc::ConMgr::SetFatal()
{
	m_is_fatal = true;
	FreeAllCon();
}

acc::ADClientCon * acc::ConMgr::FindADClientCon(const SessionId &id) const
{
	//L_DEBUG("m_vec_con.size()=%d", m_vec_con.size());
	L_COND_R(id.acc_id < m_vec_con.size(), nullptr, "acc_id overload %d", id.acc_id);

	return m_vec_con[id.acc_id];
}

const acc::Session * acc::ConMgr::FindSession(const SessionId &id) const
{
	ADClientCon *con = FindADClientCon(id);
	if (nullptr == con)
	{
		return nullptr;
	}
	return con->FindSession(id);
}

void acc::ConMgr::SetRegResult(bool is_success)
{
	if (!is_success)
	{
		m_facade.OnRegResult(0);
		return;
	}

	if (!m_is_reg)
	{
		L_COND(m_svr_id);
		m_is_reg = true;
		m_facade.OnRegResult(m_svr_id);
	}
}

void acc::ConMgr::SetHeartbeatInfo(uint32 cmd, uint32 rsp_cmd, uint64 interval_sec)
{
	m_heartbeat_info.cmd = cmd;
	m_heartbeat_info.rsp_cmd = rsp_cmd;
	m_heartbeat_info.interval_sec = interval_sec;

	for (ADClientCon *p : m_vec_con)
	{
		if (!p->IsConnect())
		{
			continue;
		}
		if (m_heartbeat_info.cmd == 0)
		{
			continue;
		}
		//L_DEBUG("send heartbeat info to port =%d", p->GetRemotePort());
		p->Send(CMD_REQ_SET_HEARTBEAT_INFO, m_heartbeat_info);
	}
}

void acc::ConMgr::FreeAllCon()
{
	L_DEBUG("FreeAllCon acc");
	for (ADClientCon *v : m_vec_con)
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
