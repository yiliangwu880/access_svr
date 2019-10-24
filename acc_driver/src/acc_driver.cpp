
#include "acc_driver.h"
#include "../acc_proto/include/proto.h"
#include "log_def.h"
#include "connect.h"

using namespace std;
using namespace lc;
using namespace su;
using namespace acc;



acc::AccFacadeMgr::AccFacadeMgr()
	:m_con_mgr(*(new ConMgr(*this)))
{
}

acc::AccFacadeMgr::~AccFacadeMgr()
{
	delete &m_con_mgr;
}

bool acc::AccFacadeMgr::Init(const std::vector<Addr> &vec_addr, uint16 svr_id)
{

	return m_con_mgr.Init(vec_addr, svr_id);
}

bool acc::AccFacadeMgr::AddAcc(const Addr &addr)
{
	return m_con_mgr.AddAcc(addr);
}

void acc::AccFacadeMgr::SetHeartbeatInfo(uint32 cmd, uint32 rsp_cmd, uint64 interval_sec)
{
	MsgReqSetHeartbeatInfo req;
	req.cmd = cmd;
	req.rsp_cmd = rsp_cmd;
	req.interval_sec = interval_sec;
	const std::vector<AccClientCon *> &vec = m_con_mgr.GetAllCon();
	for (AccClientCon *p : vec)
	{
		L_COND(p);
		p->Send(CMD_REQ_SET_HEARTBEAT_INFO, req);
	}
}

void acc::AccFacadeMgr::ReqVerifyRet(const SessionId &id, bool is_success)
{
	AccClientCon *con = m_con_mgr.FindAccClientCon(id);
	L_COND(con);

	MsgReqVerifyRet req;
	req.cid = id.cid;
	req.is_success = is_success;
	con->Send(CMD_REQ_VERIFY_RET, req);
}

void acc::AccFacadeMgr::SendToClient(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	AccClientCon *con = m_con_mgr.FindAccClientCon(id);
	L_COND(con);
	L_COND(nullptr != con->FindSession(id), "can't find session");

	MsgForward req;
	req.cid = id.cid;
	req.cmd = cmd;
	req.msg = msg;
	req.msg_len = msg_len;

	string tcp_pack;
	L_COND(ASMsg::Serialize(CMD_NTF_VERIFY_REQ, req, tcp_pack));

	con->SendPack(tcp_pack.c_str(), tcp_pack.length());
}

void acc::AccFacadeMgr::BroadCastToClient(uint32 cmd, const char *msg, uint16 msg_len)
{
	MsgReqBroadCast req;
	req.cid_len = 0;
	req.cid_s = nullptr;
	req.cmd = cmd;
	req.msg = msg;
	req.msg_len = msg_len;

	string req_tcp_pack;
	req.Serialize(req_tcp_pack);

	ASMsg as_msg;
	as_msg.cmd = CMD_REQ_BROADCAST;
	as_msg.msg = req_tcp_pack.c_str();
	as_msg.msg_len = req_tcp_pack.length();
	string tcp_pack;
	as_msg.Serialize(tcp_pack);

	const std::vector<AccClientCon *> &vec = m_con_mgr.GetAllCon();
	for (AccClientCon *p : vec)
	{
		L_COND(p);
		p->Send(as_msg);
	}
}

void acc::AccFacadeMgr::BroadCastToClient(const std::vector<uint64> &vec_cid, uint32 cmd, const char *msg, uint16 msg_len)
{
	MsgReqBroadCast req;
	req.cid_len = vec_cid.size();
	req.cid_s = &(vec_cid[0]);
	req.cmd = cmd;
	req.msg = msg;
	req.msg_len = msg_len;

	string req_tcp_pack;
	req.Serialize(req_tcp_pack);

	ASMsg as_msg;
	as_msg.cmd = CMD_REQ_BROADCAST;
	as_msg.msg = req_tcp_pack.c_str();
	as_msg.msg_len = req_tcp_pack.length();
	string tcp_pack;
	as_msg.Serialize(tcp_pack);

	const std::vector<AccClientCon *> &vec = m_con_mgr.GetAllCon();
	for (AccClientCon *p : vec)
	{
		L_COND(p);
		p->Send(as_msg);
	}
}

void acc::AccFacadeMgr::DisconClient(const SessionId &id)
{
	AccClientCon *con = m_con_mgr.FindAccClientCon(id);
	L_COND(con);
	L_COND(nullptr != con->FindSession(id), "can't find session");

	MsgReqDiscon req;
	req.cid = id.cid;
	con->Send(CMD_REQ_DISCON, req);
}

void acc::AccFacadeMgr::DisconAllClient()
{
	const std::vector<AccClientCon *> &vec = m_con_mgr.GetAllCon();
	for (AccClientCon *p : vec)
	{
		L_COND(p);
		MsgReqDiscon no_use;
		p->Send(CMD_REQ_DISCON_ALL, no_use);
	}
}

void acc::AccFacadeMgr::SetMainCmd2Svr(const SessionId &id, uint16 main_cmd, uint16 svr_id)
{
	AccClientCon *con = m_con_mgr.FindAccClientCon(id);
	L_COND(con);
	L_COND(nullptr != con->FindSession(id), "can't find session");

	MsgReqSetMainCmd2Svr req;
	req.cid = id.cid;
	req.main_cmd = main_cmd;
	req.svr_id = svr_id;
	con->Send(CMD_REQ_SET_MAIN_CMD_2_SVR, req);
}
