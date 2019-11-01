
#include "acc_driver.h"
#include "../acc_proto/include/proto.h"
#include "log_def.h"
#include "connect.h"

using namespace std;
using namespace lc;
using namespace su;
using namespace acc;



acc::ADFacadeMgr::ADFacadeMgr()
	:m_con_mgr(*(new ConMgr(*this)))
{
	
}

acc::ADFacadeMgr::~ADFacadeMgr()
{
	delete &m_con_mgr;
}

bool acc::ADFacadeMgr::Init(const std::vector<Addr> &vec_addr, uint16 svr_id, bool is_verify_svr)
{
	event_base *p = EventMgr::Obj().GetEventBase();
	L_COND_F(p, "must call EventMgr::Obj().Init before this function"); 
	return m_con_mgr.Init(vec_addr, svr_id, is_verify_svr);
}

bool acc::ADFacadeMgr::AddAcc(const Addr &addr)
{
	return m_con_mgr.AddAcc(addr);
}

void acc::ADFacadeMgr::SetHeartbeatInfo(uint32 cmd, uint32 rsp_cmd, uint64 interval_sec)
{
	m_con_mgr.SetHeartbeatInfo(cmd, rsp_cmd, interval_sec);
}

bool acc::ADFacadeMgr::ReqVerifyRet(const SessionId &id, bool is_success, uint32 cmd, const char *msg, uint16 msg_len)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	L_COND_F(msg);

	MsgReqVerifyRet req;
	req.cid = id.cid;
	req.is_success = is_success;
	
	req.forward_msg.cid = id.cid;
	req.forward_msg.cmd = cmd;
	req.forward_msg.msg = msg;
	req.forward_msg.msg_len = msg_len;

	string as_msg;
	req.Serialize(as_msg);
	string tcp_pack;
	ASMsg::Serialize(CMD_REQ_VERIFY_RET, as_msg, tcp_pack);
	return con->SendPack(tcp_pack);
}

bool acc::ADFacadeMgr::SendToClient(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	L_COND_F(nullptr != con->FindSession(id), "can't find session");

	MsgForward req;
	req.cid = id.cid;
	req.cmd = cmd;
	req.msg = msg;
	req.msg_len = msg_len;

	string tcp_pack;
	L_COND_F(ASMsg::Serialize(CMD_REQ_FORWARD, req, tcp_pack));

	return con->SendPack(tcp_pack.c_str(), tcp_pack.length());
}

void acc::ADFacadeMgr::BroadCastToClient(uint32 cmd, const char *msg, uint16 msg_len)
{
	MsgReqBroadCast req;
	req.cid_len = 0;
	req.cid_s = nullptr;
	req.cmd = cmd;
	req.msg = msg;
	req.msg_len = msg_len;

	string req_tcp_pack;
	req.Serialize(req_tcp_pack);
	string tcp_pack;
	L_COND(ASMsg::Serialize(CMD_REQ_BROADCAST, req_tcp_pack, tcp_pack));

	const std::vector<ADClientCon *> &vec = m_con_mgr.GetAllCon();
	for (ADClientCon *p : vec)
	{
		L_COND(p);
		if (!p->IsReg())
		{
			continue;
		}
		p->SendPack(tcp_pack);
	}
}

void acc::ADFacadeMgr::BroadCastToClient(const std::vector<uint64> &vec_cid, uint32 cmd, const char *msg, uint16 msg_len)
{
	MsgReqBroadCast req;
	req.cid_len = vec_cid.size();
	req.cid_s = &(vec_cid[0]);
	req.cmd = cmd;
	req.msg = msg;
	req.msg_len = msg_len;

	string req_tcp_pack;
	req.Serialize(req_tcp_pack);

	string tcp_pack;
	L_COND(ASMsg::Serialize(CMD_REQ_BROADCAST, req_tcp_pack, tcp_pack));

	const std::vector<ADClientCon *> &vec = m_con_mgr.GetAllCon();
	for (ADClientCon *p : vec)
	{
		L_COND(p);
		if (!p->IsReg())
		{
			continue;
		}
		p->SendPack(tcp_pack);
	}
}

bool acc::ADFacadeMgr::DisconClient(const SessionId &id)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	L_COND_F(nullptr != con->FindSession(id), "can't find session");

	MsgReqDiscon req;
	req.cid = id.cid;
	return con->Send(CMD_REQ_DISCON, req);
}

void acc::ADFacadeMgr::DisconAllClient()
{
	const std::vector<ADClientCon *> &vec = m_con_mgr.GetAllCon();
	for (ADClientCon *p : vec)
	{
		L_COND(p);
		if (!p->IsReg())
		{
			continue;
		}
		MsgReqDiscon no_use;
		p->Send(CMD_REQ_DISCON_ALL, no_use);
	}
}

bool acc::ADFacadeMgr::SetMainCmd2Svr(const SessionId &id, uint16 main_cmd, uint16 svr_id)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	if (nullptr == con->FindSession(id))
	{
		L_INFO("can't find session");
		return false;
	}

	MsgReqSetMainCmd2Svr req;
	req.cid = id.cid;
	req.main_cmd = main_cmd;
	req.svr_id = svr_id;
	return con->Send(CMD_REQ_SET_MAIN_CMD_2_SVR, req);
}

void acc::ADFacadeMgr::OnSetMainCmd2SvrRsp(const Session &session, uint16 main_cmd, uint16 svr_id)
{

}

void acc::ADFacadeMgr::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	L_WARN("OnRevVerifyReq no implement");
}

void acc::ADFacadeMgr::OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len)
{
	L_WARN("OnRevClientMsg no implement");
}

void acc::ADFacadeMgr::OnClientDisCon(const Session &session)
{

}

void acc::ADFacadeMgr::OnClientConnect(const Session &session)
{

}

void acc::ADFacadeMgr::OnAccDisCon(const std::string &ip, uint16 port)
{
	L_INFO("acc disconnect. ip=%s, port=%d", ip.c_str(), port);
}
