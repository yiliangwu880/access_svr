
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
	event_base *p = EventMgr::Ins().GetEventBase();
	L_COND_F(p, "must call EventMgr::Ins().Init before this function"); 
	return m_con_mgr.Init(vec_addr, svr_id, is_verify_svr);
}

bool acc::ADFacadeMgr::AddAcc(const Addr &addr)
{
	return m_con_mgr.AddAcc(addr);
}



void acc::ADFacadeMgr::SetAccSeting(const MsgAccSeting &seting)
{
	m_con_mgr.SetAccSeting(seting);
}

bool acc::ADFacadeMgr::ReqVerifyRet(const SessionId &id, const VerifyRetStruct &d)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	L_COND_F(d.msg);

	MsgReqVerifyRet req;
	req.cid = id.cid;
	req.is_success = d.is_success;
	req.uin = d.uin;
	req.accName = d.accName;
	
	req.rsp_msg.cmd = d.cmd;
	req.rsp_msg.msg = d.msg;
	req.rsp_msg.msg_len = d.msg_len;

	string as_msg;
	req.Serialize(as_msg);
	string tcp_pack;
	ASMsg::Serialize(CMD_REQ_VERIFY_RET, as_msg, tcp_pack);
	return con->SendPack(tcp_pack);
}

bool acc::ADFacadeMgr::ReqVerifyRet(const SessionId &id, bool is_success, uint32 cmd, const char *msg, uint16 msg_len)
{
	VerifyRetStruct d;
	d.cmd = cmd;
	d.msg = msg;
	d.msg_len = (uint16)msg_len;
	return ReqVerifyRet(id, d);
}

bool acc::ADFacadeMgr::BroadcastUinToSession(const SessionId &id, uint64 uin)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	L_COND_F(nullptr != con->FindSession(id), "can't find session");
	MsgBroadcastUin req;
	req.uin = uin;
	req.cid = id.cid;

	string tcp_pack;
	L_COND_F(ASMsg::Serialize(CMD_REQ_BROADCAST_UIN, req, tcp_pack));

	return con->SendPack(tcp_pack.c_str(), tcp_pack.length());
}

bool acc::ADFacadeMgr::SendToClient(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	L_COND_F(nullptr != con->FindSession(id), "can't find session");//todo 待优化，不用查。acc检查失败，通知删掉session

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
	req.broadcast_msg.cmd = cmd;
	req.broadcast_msg.msg = msg;
	req.broadcast_msg.msg_len = msg_len;

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
	req.broadcast_msg.cmd = cmd;
	req.broadcast_msg.msg = msg;
	req.broadcast_msg.msg_len = msg_len;

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


void acc::ADFacadeMgr::SetMainCmd2GrpId(uint16 grpId, const std::vector<uint16> &vecCmd)
{
	//const std::vector<ADClientCon *> &vec = m_con_mgr.GetAllCon();
	for (ADClientCon *p : m_con_mgr.GetAllCon())
	{
		L_COND(p);
		if (!p->IsReg())
		{
			L_ERROR("SetMainCmd2GrpId find UnReg acc connect");
			continue;
		}
		MsgReqSetCmd2GrpId req;
		req.grpId = grpId;
		req.vecCmd = vecCmd;
		p->Send(CMD_REQ_SET_CMD_2_GRP, req);
	}
}

bool acc::ADFacadeMgr::SetActiveSvrId(const SessionId &id, uint16 grpId, uint16 svrId)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	if (nullptr == con->FindSession(id))
	{
		L_INFO("can't find session");
		return false;
	}

	MsgReqSetActiveSvrId req;
	req.cid = id.cid;
	req.grpId = grpId;
	req.svrId = svrId;
	return con->Send(CMD_REQ_SET_ACTIVE_SVR, req);
}

bool acc::ADFacadeMgr::SetCache(const SessionId &id, bool isCache)
{
	ADClientCon *con = m_con_mgr.FindADClientCon(id);
	L_COND_F(con);
	L_COND_F(con->IsReg());
	if (nullptr == con->FindSession(id))
	{
		L_INFO("can't find session");
		return false;
	}

	MsgReqCacheMsg req;
	req.cid = id.cid;
	req.isCache = isCache;
	return con->Send(CMD_REQ_CACHE_MSG, req);
}

const Session *acc::ADFacadeMgr::FindSession(const SessionId &id)
{
	return m_con_mgr.FindSession(id);
}

const acc::Session * acc::ADFacadeMgr::FindSessionByCid(uint64 cid)
{
	return m_con_mgr.FindSessionByCid(cid);
}

void acc::ADFacadeMgr::OnSetActiveSvr(const Session &session, uint16 grpId, uint16 svr_id)
{

}

void acc::ADFacadeMgr::OnMsgRspCacheMsg(const Session &session, bool isCache)
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

void acc::Session::Clear()
{
	id.acc_id = 0;
	id.cid = 0;
	remote_ip.clear();
	remote_port = 0;
	uin = 0;
}
