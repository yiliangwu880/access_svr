#include <memory>
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

	SetMaxSendBufSize(ACC_SVR_MAX_SEND_BUF_SIZE);
}

void ADClientCon::OnRecv(const lc::MsgPack &msg)
{
	ASMsg as_data;
	L_COND(as_data.Parse(msg.data, msg.len));

	if (CMD_RSP_REG != as_data.cmd)
	{
		L_COND(m_is_reg, "must reg ok before rev other cmd %d. acc port=%d", as_data.cmd, GetRemotePort());//svr处理非CMD_RSP_REG消息,必须已注册。
	}

	switch (as_data.cmd)
	{
	case CMD_RSP_REG				:HandleRspReg(as_data)              ; return;
	case CMD_NTF_CREATE_SESSION		:HandleCreateSession(as_data)       ; return;
	case CMD_NTF_VERIFY_REQ			:HandleVerifyReq(as_data)           ; return;
	case CMD_NTF_FORWARD			:HandleMsgForward(as_data)          ; return;
	case CMD_NTF_DISCON				:HandleMsgNtfDiscon(as_data)        ; return;
	case CMD_RSP_SET_ACTIVE_SVR:HandleMsgRspSetActiveSvr(as_data); return;
	case CMD_RSP_CACHE_MSG:HandleMsgRspCacheMsg(as_data); return;
	case CMD_RSP_BROADCAST_UIN		:HandleMsgBroadcastUin(as_data); return;
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

	if (const MsgAccSeting *req = m_con_mgr.GetMsgAccSeting())
	{
		L_DEBUG("OnConnected. send MsgAccSeting info. %d %d %d no_msg_interval_sec=%d, remote port=%d",
			req->hbi.req_cmd, req->hbi.rsp_cmd, req->hbi.interval_sec,
			req->no_msg_interval_sec, GetRemotePort());
		string as_msg;
		L_COND(req->Serialize(as_msg));
		Send(CMD_REQ_ACC_SETING, as_msg);
	}
}

void acc::ADClientCon::OnDisconnected()
{
	L_DEBUG("OnDisconnected, start try recon timer , sec=%d", RE_CON_INTERVAL_SEC);
	m_is_reg = false;
	m_id_2_s.clear();
	auto f = std::bind(&ADClientCon::OnTryReconTimeOut, this);
	m_recon_tm.StopTimer();
	m_recon_tm.StartTimer(RE_CON_INTERVAL_SEC*1000, f);
	string ip;
	uint16 port=0;
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
//	L_DEBUG("cmd =%d tcp_pack.length %d", as_data.cmd, tcp_pack.length());
	return SendPack(tcp_pack);
}

bool acc::ADClientCon::Send(acc::Cmd as_cmd, const std::string &as_msg)
{
	std::string tcp_pack;
	L_COND_F(ASMsg::Serialize(as_cmd, as_msg, tcp_pack));
//	L_DEBUG("cmd =%d tcp_pack.length %d", as_cmd, tcp_pack.length());
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

	auto pair = m_id_2_s.insert(make_pair(rsp.cid, Session()));
	bool r = pair.second;
	if (!r)
	{
		L_ERROR("create session fail, repeated insert id. cid=%lld acc_id=%d", s.id.cid, s.id.acc_id);
		return;
	}
	Session &s = (pair.first->second);
	s.id.cid = rsp.cid;
	s.uin = rsp.uin;
	s.accName = rsp.accName;
	s.id.acc_id = m_acc_id;
	s.remote_ip = inet_ntoa(rsp.addr.sin_addr);
	s.remote_port = ntohs(rsp.addr.sin_port);
	m_facade.OnClientConnect(s);
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
	//这里还没有做会话查找，等用户需要发送消息到client再查找.
	m_facade.OnRevClientMsg(it->second, f_msg.cmd, f_msg.msg, f_msg.msg_len);
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

	auto it = m_id_2_s.find(id.cid);
	if (it == m_id_2_s.end())
	{
		L_ERROR("can't find session when rev ntf discon. %lld %d", id.cid, id.acc_id);
		return;
	}
	m_facade.OnClientDisCon(it->second);
	m_id_2_s.erase(it);
}



void acc::ADClientCon::HandleMsgRspSetActiveSvr(const ASMsg &msg)
{
	MsgRspSetActiveSvrId rsp;
	bool ret = CtrlMsgProto::Parse(msg, rsp);
	L_COND(ret, "parse ctrl msg fail");
	L_COND(rsp.cid);
	SessionId id;
	id.cid = rsp.cid;
	id.acc_id = m_acc_id;
	auto it = m_id_2_s.find(id.cid);
	if (it == m_id_2_s.end())
	{
		L_ERROR("can't find session. %lld %d", id.cid, id.acc_id);
		return;
	}

	m_facade.OnSetActiveSvr(it->second, rsp.grpId, rsp.svrId);
}
void acc::ADClientCon::HandleMsgRspCacheMsg(const ASMsg &msg)
{
	MsgRspCacheMsg rsp;
	bool ret = CtrlMsgProto::Parse(msg, rsp);
	L_COND(ret, "parse ctrl msg fail");
	L_COND(rsp.cid);
	SessionId id;
	id.cid = rsp.cid;
	id.acc_id = m_acc_id;
	auto it = m_id_2_s.find(id.cid);
	if (it == m_id_2_s.end())
	{
		L_ERROR("can't find session. %lld %d", id.cid, id.acc_id);
		return;
	}

	m_facade.OnMsgRspCacheMsg(it->second, rsp.isCache);
}

void acc::ADClientCon::HandleMsgBroadcastUin(const ASMsg &msg)
{
//	L_DEBUG("HandleMsgBroadcastUin");
	MsgBroadcastUin rsp;
	bool ret = CtrlMsgProto::Parse(msg, rsp);
	L_COND(ret, "parse ctrl msg fail");
	L_COND(rsp.cid);
	SessionId id;
	id.cid = rsp.cid;
	id.acc_id = m_acc_id;
	auto it = m_id_2_s.find(id.cid);
	if (it == m_id_2_s.end())
	{
		L_WARN("can't find session. %lld %d", id.cid, id.acc_id);
		return;
	}
	Session &sn = it->second;
	sn.uin = rsp.uin;
	//L_DEBUG("cb OnRevBroadcastUinToSession ");
	m_facade.OnRevBroadcastUinToSession(sn);
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


void acc::ConMgr::SetAccSeting(const MsgAccSeting &seting)
{
	m_seting = std::make_unique<MsgAccSeting>();
	*m_seting = seting;
	for (ADClientCon *p : m_vec_con)
	{
		if (!p->IsConnect())
		{
			continue;
		}
		//L_DEBUG("send heartbeat info to port =%d", p->GetRemotePort());
		string as_msg;
		L_COND(m_seting->Serialize(as_msg));
		p->Send(CMD_REQ_ACC_SETING, as_msg);
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
