#include "external_con.h"
#include "inner_con.h"
#include "com.h"
#include "server.h"

using namespace std;
using namespace acc;
using namespace lc;

ExternalSvrCon::ExternalSvrCon()
	:m_state(State::INIT)
	, m_uin(0)
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
		L_DEBUG("external client destroy . cid=%llx", GetId());
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

void ExternalSvrCon::OnConnected()
{
	//L_DEBUG("ExternalSvrCon OnConnected . sec=%d", AccSeting::Obj().m_seting.no_msg_interval_sec);
	const ClientLimitInfo &cli = AccSeting::Obj().m_seting.cli;
	if (Server::Obj().GetExConSize() > cli.max_num)
	{
		L_INFO("external con too much. cur size=%d. rsp_cmd=%d", Server::Obj().GetExConSize(), cli.rsp_cmd);
		SendMsg(cli.rsp_cmd, cli.rsp_msg.c_str(), cli.rsp_msg.length());
		//延时断开，等上面消息发送出去。这个方法不可靠，可能发送失败。 以后再想办法。
		auto f = std::bind(&ExternalSvrCon::DisConnect, this);
		m_cls_tm.StartTimer(1000, f);
		return;
	}

	if (0 != AccSeting::Obj().m_seting.no_msg_interval_sec)
	{
		//L_DEBUG("start no msg timer . sec=%d", AccSeting::Obj().m_seting.no_msg_interval_sec);
		auto f = std::bind(&ExternalSvrCon::OnWaitFirstMsgTimeOut, this);
		m_wfm_tm.StartTimer(AccSeting::Obj().m_seting.no_msg_interval_sec * 1000, f);
	}
}

//client 接收的tcp pack 转 MsgForward
bool ExternalSvrCon::ClientTcpPack2MsgForward(const lc::MsgPack &msg, MsgForward &f_msg) const
{
	L_COND_F(msg.len>=sizeof(f_msg.cmd));
	f_msg.cid = GetId();
	const char *cur = msg.data;
	f_msg.cmd = *(decltype(f_msg.cmd)*)(cur);	cur = cur + sizeof(f_msg.cmd);
	f_msg.msg = cur;
	f_msg.msg_len = msg.len - sizeof(f_msg.cmd);
	return true;
}

void ExternalSvrCon::Forward2VerifySvr(const lc::MsgPack &msg)
{
	L_COND(State::INIT == m_state);

	m_wfm_tm.StopTimer();
	string tcp_pack;
	{//client tcp pack to ASData tcp pack 
		MsgForward f_msg;
		//L_DEBUG("msg.data first 32bit = %x", *(uint32*)msg.data);
		if (!ClientTcpPack2MsgForward(msg, f_msg))
		{
			L_WARN("client illegal msg");
			return;
		}
		//L_DEBUG("f_msg.cmd = %x", f_msg.cmd);
		L_COND(ASMsg::Serialize(CMD_NTF_VERIFY_REQ, f_msg, tcp_pack));
	}

	InnerSvrCon *pSvr = VerifySvrMgr::Obj().GetBLVerifySvr();
	if (nullptr == pSvr)
	{
		L_WARN("can't find verfify svr. maybe you havn't reg your verify svr.");
		DisConnect();
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
	L_WARN("wait verify time out, disconnect client");//通常验证服务器异常，导致没验证响应。当连接失败处理
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
	const HeartBeatInfo &hbi = AccSeting::Obj().m_seting.hbi;
	if (hbi.req_cmd != 0 && hbi.req_cmd == f_msg.cmd)
	{//reset heartbeat
		//如果这样做法效率不满意，试下 循环定时器，过期时间里面检查心跳状态是否没更新，没更新就断开。 这样让用户层不用创建销毁定时器。
		m_heartbeat_tm.StopTimer();
		L_ASSERT(0 != hbi.interval_sec);
		auto f = std::bind(&ExternalSvrCon::OnHeartbeatTimeOut, this);
		L_DEBUG("rev beat, start time sec=%d", hbi.interval_sec);
		m_heartbeat_tm.StartTimer(hbi.interval_sec*1000, f);
		SendMsg(hbi.rsp_cmd, "", 0);
		return;
	}
	//真正处理转发
	uint16 main_cmd = f_msg.cmd >> 16;
	uint16 svr_id = main_cmd;
	{//get svr_id
		auto it = m_cmd_2_svrid.find(main_cmd);
		if (it != m_cmd_2_svrid.end())
		{
			svr_id = it->second;
		}
	}
	if (0 == svr_id)
	{
		L_WARN("invaild cmd %x. hight uint16 must can't be 0", f_msg.cmd);
		DisConnect();
		return;
	}

	//L_DEBUG("f_msg.cmd=%x HeartbeatInfo::Obj().cmd=%x %x", f_msg.cmd, AccSeting::Obj().m_seting.hbi.req_cmd, AccSeting::Obj().m_seting.hbi.rsp_cmd);
	InnerSvrCon *pSvr = RegSvrMgr::Obj().Find(svr_id);
	if (nullptr == pSvr)
	{
		L_WARN("client req can't find verfify svr. cmd=%x svr_id=%d main_cmd=%d", f_msg.cmd, svr_id, main_cmd);
		return;
	}

	string tcp_pack;
	L_COND(ASMsg::Serialize(CMD_NTF_FORWARD, f_msg, tcp_pack));
	pSvr->SendPack(tcp_pack.c_str(), tcp_pack.length());

}

void ExternalSvrCon::OnWaitFirstMsgTimeOut()
{
	L_INFO("wait client 1st msg time out, disconnect");
	DisConnect();
}

