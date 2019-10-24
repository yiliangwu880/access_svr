#include "client_con.h"
#include "svr_con.h"
#include "com.h"

using namespace std;
using namespace acc;
using namespace lc;

ExternalSvrCon::ExternalSvrCon()
	:m_state(State::INIT)
{
}

ExternalSvrCon::~ExternalSvrCon()
{
	if (IsVerify())
	{
		MsgNtfDiscon ntf;
		ntf.cid = GetId();
		L_ASSERT(ntf.cid);
		const Id2Svr &id_2_svr = RegSvrMgr::Obj().GetRegSvr();
		for (auto &v: id_2_svr)
		{
			InnerSvrCon *p = v.second;
			L_COND(p);
			p->Send(CMD_NTF_DISCON, ntf);
		}
	}
}

void ExternalSvrCon::SetVerify(bool is_success)
{
	if (m_state != State::WAIT_VERIFY)
	{
		//svr请求验证结果 状态时机不对
		L_WARN("svr req CMD_REQ_VERIFY_RET state not correct");
		return;
	}
	if (is_success)
	{
		m_state = State::VERIFYED;
	}
	else
	{
		m_state = State::INIT;//认证失败，等客户端再次请求认证
	}
	m_verify_tm.StopTimer();
}

bool ExternalSvrCon::IsVerify()
{
	return State::VERIFYED == m_state;
}

bool ExternalSvrCon::SendMsg(uint32 cmd, const char *msg, uint16 msg_len)
{
	string s;
	s.append((char *)&cmd, sizeof(cmd));
	s.append(msg, msg_len);
	return SendPack(s.c_str(), s.length());
}

void ExternalSvrCon::SetMainCmd2SvrId(uint16 main_cmd, uint16 svr_id)
{
	L_COND(0 != main_cmd);
	L_COND(0 != svr_id);
	m_cmd_2_svrid[main_cmd] = svr_id;
}

void ExternalSvrCon::OnRecv(const lc::MsgPack &msg)
{
	switch (m_state)
	{
	default:
		L_ERROR("unknow state %d", (int)m_state);
		break;
	case ExternalSvrCon::State::INIT:
		Forward2VerifySvr(msg);
		break;
	case ExternalSvrCon::State::WAIT_VERIFY:
		L_WARN("client repeated req verify, ignore");
		break;
	case ExternalSvrCon::State::VERIFYED:
		Forward2Svr(msg);
		break;
	}
	return;
}

//client 接收的tcp pack 转 MsgForward
bool ExternalSvrCon::ClientTcpPack2MsgForward(const lc::MsgPack &msg, MsgForward &f_msg) const
{
	L_COND_F(msg.len>=sizeof(f_msg.cmd));
	f_msg.cid = GetId();
	const char *cur = msg.data;
	f_msg.cmd = (decltype(f_msg.cmd))(*cur);	cur = cur + sizeof(f_msg.cmd);
	f_msg.msg = cur;
	f_msg.msg_len = msg.len - sizeof(f_msg.cmd);
	return true;
}

void ExternalSvrCon::Forward2VerifySvr(const lc::MsgPack &msg)
{
	L_COND(State::INIT == m_state);

	string tcp_pack;
	{//client tcp pack to ASData tcp pack 
		MsgForward f_msg;
		if (!ClientTcpPack2MsgForward(msg, f_msg))
		{
			L_WARN("client illegal msg");
			return;
		}
		L_COND(ASMsg::Serialize(CMD_NTF_FORWARD, f_msg, tcp_pack));
	}

	InnerSvrCon *pSvr = VerifySvrMgr::Obj().GetBLVerifySvr();
	if (nullptr == pSvr)
	{
		L_WARN("can't find verfify svr. maybe you havn't reg your verify svr.");
		return;
	}

	pSvr->SendPack(tcp_pack.c_str(), tcp_pack.length());

	m_verify_tm.StopTimer();
	auto f = std::bind(&ExternalSvrCon::OnVerfiyTimeOut, this);
	m_verify_tm.StartTimer(VERIFY_TIME_OUT_SEC*1000, f);
	m_state = State::WAIT_VERIFY;
}

void ExternalSvrCon::OnVerfiyTimeOut()
{
	L_ASSERT(State::WAIT_VERIFY == m_state);
	L_WARN("wait verify time out, disconnect client");//通常验证服务器异常，导致没验证响应。
	DisConnect();
}

void ExternalSvrCon::OnHeartbeatTimeOut()
{
	L_DEBUG("OnHeartbeatTimeOut, disconnect client cid=%lld", GetId());
	DisConnect();
}

void ExternalSvrCon::Forward2Svr(const lc::MsgPack &msg)
{
	MsgForward f_msg;
	if (!ClientTcpPack2MsgForward(msg, f_msg))
	{
		L_WARN("client illegal msg");
		return;
	}
	if (HeartbeatInfo::Obj().cmd != 0 && HeartbeatInfo::Obj().cmd == f_msg.cmd)
	{//reset heartbeat
		m_heartbeat_tm.StopTimer();
		L_ASSERT(0 != HeartbeatInfo::Obj().interval_sec);
		auto f = std::bind(&ExternalSvrCon::OnHeartbeatTimeOut, this);
		m_heartbeat_tm.StartTimer(HeartbeatInfo::Obj().interval_sec, f);
		return;
	}
	//真正处理转发
	uint16 svr_id = 0;
	uint16 main_cmd = f_msg.cmd >> 16;
	{//get svr_id
		auto it = m_cmd_2_svrid.find(main_cmd);
		if (it == m_cmd_2_svrid.end())
		{			svr_id = main_cmd;
		}
		else
		{
			svr_id = it->second;
		}
	}


	InnerSvrCon *pSvr = RegSvrMgr::Obj().Find(svr_id);
	if (nullptr == pSvr)
	{
		L_WARN("client req can't find verfify svr. svr_id=%d main_cmd=%d", svr_id, main_cmd);
		return;
	}

	string tcp_pack;
	L_COND(ASMsg::Serialize(CMD_NTF_FORWARD, f_msg, tcp_pack));
	pSvr->SendPack(tcp_pack.c_str(), tcp_pack.length());

}

