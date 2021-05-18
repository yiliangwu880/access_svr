#include "inner_con.h"
#include <utility>
#include "external_con.h"
#include "com.h"
#include "server.h"

using namespace std;
using namespace acc;
using namespace lc;


namespace
{

	void Parse_CMD_REQ_ACC_SETING(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgAccSeting req;
		bool ret = req.Parse(msg.msg, msg.msg_len);
		L_COND(ret, "parse ctrl msg fail");
		L_INFO("rev CMD_REQ_ACC_SETING");
//		L_DEBUG("req no msg sec=%d", req.no_msg_interval_sec);
		AccSeting::Ins().m_seting = req;
	}

	void Parse_CMD_REQ_DISCON_ALL(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		L_INFO("svr req disconnect all client");
		BaseConMgr &conMgr = Server::Ins().m_client_listener.GetConnMgr();
		//L_DEBUG("con size=%d, wait del size=%d", conMgr.GetAllCon().size(), conMgr.GetWaitDelConSize());
		auto f=[](SvrCon &svr_con)
		{
			//L_DEBUG("DisConnect cid=%llx", svr_con.GetId());
			svr_con.DisConnect();
		};
		conMgr.Foreach(f);
	}

	void Parse_CMD_REQ_DISCON(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqDiscon req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		if (req.cid==0)
		{
			L_WARN("CMD_REQ_DISCON cid==0");
			return;
		}
		ExternalSvrCon *pClient = Server::Ins().FindClientSvrCon(req.cid);
		L_COND(pClient);
		pClient->DisConnect();
	}

	void Parse_CMD_REQ_REG(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqReg req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		MsgRspReg rsp;
		rsp.svr_id = req.svr_id;
		L_DEBUG("Parse_CMD_REQ_REG svr_id=%x, is_verify=%d", req.svr_id, req.is_verify);
		if (!con.RegSvrId(req.svr_id))
		{
			L_WARN("reg fail, svr id=%d", req.svr_id);
			rsp.svr_id = 0;
		}
		if (req.is_verify)
		{
			con.SetVerifySvr();
		}
		con.Send(CMD_RSP_REG, rsp);
		con.RevertSession();
	}

	void Parse_CMD_REQ_VERIFY_RET(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqVerifyRet req;
		L_COND(req.Parse(msg.msg, msg.msg_len), "parse ctrl msg fail");
		L_COND(req.cid != 0, "CMD_REQ_VERIFY_RET cid==0");
		const ClientSvrMsg &f_msg = req.rsp_msg;

		ExternalSvrCon *pClient = Server::Ins().FindClientSvrCon(req.cid);
		if (nullptr == pClient)
		{
			L_DEBUG("find cid fail. cid=%lld", req.cid);
			return;
		}
		//notify verify result to client
		pClient->SendMsg(f_msg.cmd, f_msg.msg, f_msg.msg_len);
		if (!req.is_success)
		{
			return;
		}
		if (pClient->IsVerify())
		{
			L_DEBUG("repeated verify"); //客户端重复请求验证，重复接收验证成功。
			return;
		}

		pClient->SetVerify(req.is_success);

		//通知创建会话
		MsgNtfCreateSession ntf;
		ntf.cid = req.cid;
		ntf.uin = 0;
		ntf.addr = pClient->GetRemoteAddr();
		//L_DEBUG("verify create session. port=%x %x", ntohs(pClient->GetRemoteAddr().sin_port), pClient->GetRemotePort());
		auto f = [&ntf](SvrCon &con)
		{
			InnerSvrCon *pCon = dynamic_cast<InnerSvrCon *>(&con);
			L_COND(pCon);
			if (pCon->IsReg())
			{
				pCon->Send(CMD_NTF_CREATE_SESSION, ntf);
			}
		};
		Server::Ins().m_svr_listener.GetConnMgr().Foreach(f);

	}
	void Parse_CMD_REQ_BROADCAST_UIN(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgBroadcastUin req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
	//	L_DEBUG("Parse_CMD_REQ_BROADCAST_UIN uin=%llx", req.uin);
		if (req.cid == 0)
		{
			L_WARN("CMD_REQ_BROADCAST_UIN cid==0");
			return;
		}
		ExternalSvrCon *pClient = Server::Ins().FindClientSvrCon(req.cid);
		if (nullptr == pClient)
		{
			L_DEBUG("client not exist");
			return;
		}
		pClient->SetUin(req.uin);

		//通知 svr Uin
		auto f = [&req, &con](SvrCon &f_con)
		{
			InnerSvrCon *pCon = dynamic_cast<InnerSvrCon *>(&f_con);
			L_COND(pCon);
			if (pCon == &con)
			{
				return;//不发送给来源svr
			}
			if (!pCon->IsReg())
			{
				return;
			}
			//L_DEBUG("CMD_RSP_BROADCAST_UIN uin=%lld", req.uin);
			pCon->Send(CMD_RSP_BROADCAST_UIN, req);
		};
		Server::Ins().m_svr_listener.GetConnMgr().Foreach(f);
	}

	void Parse_CMD_REQ_SET_MAIN_CMD_2_SVR(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqSetMainCmd2GrpId req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		MsgRspSetMainCmd2GrpId rsp;
		rsp.cid = req.cid;
		rsp.main_cmd = req.main_cmd;
		rsp.grpId = req.grpId;
		if (req.cid == 0)
		{
			L_WARN("CMD_REQ_VERIFY_RET cid==0");
			rsp.grpId = 0;
			con.Send(CMD_RSP_SET_MAIN_CMD_2_SVR, rsp);
			return;
		}

		ExternalSvrCon *pClient = Server::Ins().FindClientSvrCon(req.cid);
		if (nullptr == pClient)
		{
			rsp.grpId = 0;
			con.Send(CMD_RSP_SET_MAIN_CMD_2_SVR, rsp);
			L_DEBUG("client not exist");
			return;
		}
		if (!pClient->IsVerify())
		{
			L_WARN("client is not verify ,can't handle CMD_RSP_SET_MAIN_CMD_2_SVR");
			return;
		}

		pClient->SetMainCmd2GrpId(req.main_cmd, req.grpId);
		con.Send(CMD_RSP_SET_MAIN_CMD_2_SVR, rsp);
	}

	void Parse_CMD_REQ_SET_ACTIVE_SVR(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqSetActiveSvrId req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		MsgRspSetActiveSvrId rsp;
		rsp.cid = req.cid;
		rsp.grpId = req.grpId;
		rsp.svrId = req.svrId;
		if (req.cid == 0)
		{
			L_WARN("CMD_REQ_VERIFY_RET cid==0");
			rsp.grpId = 0;
			rsp.svrId = 0;
			con.Send(CMD_RSP_SET_ACTIVE_SVR, rsp);
			return;
		}

		ExternalSvrCon *pClient = Server::Ins().FindClientSvrCon(req.cid);
		if (nullptr == pClient)
		{
			rsp.grpId = 0;
			rsp.svrId = 0;
			con.Send(CMD_RSP_SET_ACTIVE_SVR, rsp);
			L_DEBUG("client not exist");
			return;
		}
		if (!pClient->IsVerify())
		{
			L_WARN("client is not verify ,can't handle CMD_RSP_SET_MAIN_CMD_2_SVR");
			return;
		}

		pClient->SetActiveSvrId(req.grpId, req.svrId);
		con.Send(CMD_RSP_SET_ACTIVE_SVR, rsp);
	}

	void Parse_CMD_REQ_CACHE_MSG(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqCacheMsg req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		MsgRspCacheMsg rsp;
		rsp.cid = req.cid;
		rsp.isCache = req.isCache;
		if (req.cid == 0)
		{
			L_WARN("CMD_REQ_VERIFY_RET cid==0");
			return;
		}

		ExternalSvrCon *pClient = Server::Ins().FindClientSvrCon(req.cid);
		if (nullptr == pClient)
		{
			L_WARN("client not exist");
			return;
		}
		if (!pClient->IsVerify())
		{
			L_WARN("client is not verify ,can't handle CMD_RSP_SET_MAIN_CMD_2_SVR");
			return;
		}

		con.Send(CMD_RSP_CACHE_MSG, rsp);
		pClient->SetCache(req.isCache);
	}
	void Parse_CMD_REQ_BROADCAST(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqBroadCast req;
		L_COND(req.Parse(msg.msg, msg.msg_len), "parse ctrl msg fail");
		if (req.cid_len > acc::MAX_BROADCAST_CID_NUM)
		{
			L_WARN("CMD_REQ_BROADCAST cid_len> acc::MAX_BROADCAST_CID_NUM");
			return;
		}

		string tcp_pack;
		tcp_pack.append((char *)&req.broadcast_msg.cmd, sizeof(req.broadcast_msg.cmd));
		tcp_pack.append(req.broadcast_msg.msg, req.broadcast_msg.msg_len);
		if (req.cid_len == 0)
		{
			auto f = [&req, &tcp_pack](SvrCon &con)
			{
				ExternalSvrCon *pClient = dynamic_cast<ExternalSvrCon *>(&con);
				L_COND(pClient);
				if (pClient->IsVerify())
				{
					pClient->SendPack(tcp_pack.c_str(), tcp_pack.length());
				}
			};
			Server::Ins().m_client_listener.GetConnMgr().Foreach(f);
		}
		else
		{
			for (uint32 i=0; i<req.cid_len; ++i)
			{
				if (req.cid_s[i]==0)
				{
					L_WARN("wrong uin==0");
					continue;
				}
				ExternalSvrCon *pClient = Server::Ins().FindClientSvrCon(req.cid_s[i]);
				if (nullptr == pClient)//客户端刚刚断开，svr还没收到消息，正常。
				{
					continue;
				}
				if (!pClient->IsVerify())
				{
					L_ERROR("unverifyed client can't be broadcast, %llx", req.cid_s[i]);//哪里错误了，未认证不能设置广播
					continue;
				}
				pClient->SendPack(tcp_pack.c_str(), tcp_pack.length());
			}
		}
	}
}


void SvrCtrlMsgDispatch::Init()
{
	m_cmd_2_handle[CMD_REQ_REG]                = Parse_CMD_REQ_REG;
	m_cmd_2_handle[CMD_REQ_VERIFY_RET]         = Parse_CMD_REQ_VERIFY_RET;
	m_cmd_2_handle[CMD_REQ_BROADCAST]          = Parse_CMD_REQ_BROADCAST;
	m_cmd_2_handle[CMD_REQ_SET_MAIN_CMD_2_SVR] = Parse_CMD_REQ_SET_MAIN_CMD_2_SVR;
	m_cmd_2_handle[CMD_REQ_SET_ACTIVE_SVR]	   = Parse_CMD_REQ_SET_ACTIVE_SVR;
	m_cmd_2_handle[CMD_REQ_DISCON]             = Parse_CMD_REQ_DISCON;
	m_cmd_2_handle[CMD_REQ_DISCON_ALL]         = Parse_CMD_REQ_DISCON_ALL;
	m_cmd_2_handle[CMD_REQ_ACC_SETING]		   = Parse_CMD_REQ_ACC_SETING;
	m_cmd_2_handle[CMD_REQ_BROADCAST_UIN] = Parse_CMD_REQ_BROADCAST_UIN;
	m_cmd_2_handle[CMD_REQ_CACHE_MSG]	= Parse_CMD_REQ_CACHE_MSG;
}

void SvrCtrlMsgDispatch::DispatchMsg(InnerSvrCon &con, const acc::ASMsg &msg)
{
	auto handle_it = m_cmd_2_handle.find((acc::Cmd)msg.cmd);
	if (handle_it == m_cmd_2_handle.end())
	{
		L_ERROR("find cmd handler fail. ctrl_cmd=%d", msg.cmd);
		return;
	}

	(*handle_it->second)(con, msg);
}

InnerSvrCon::InnerSvrCon()
{
	m_svr_id = 0;
	m_is_verify = false;
	SetMaxSendBufSize(ACC_SVR_MAX_SEND_BUF_SIZE);
}

InnerSvrCon::~InnerSvrCon()
{
	L_DEBUG("destory svr, m_svr_id=%x", m_svr_id);
	if (0 != m_svr_id)
	{
		if (!RegSvrMgr::Ins().Remove(m_svr_id))
		{
			L_ERROR("SvrRegMgr::Ins().Remove fail. m_svr_id=%d", m_svr_id);
		}
		m_svr_id = 0;
	}
	if (m_is_verify)
	{
		if (!VerifySvrMgr::Ins().RemoveVerify(this))
		{
			L_ERROR("SvrRegMgr::Ins().RemoveVerify fail.");
		}
		m_is_verify = false;
	}
}

void InnerSvrCon::RevertSession()
{
	//通知创建会话
	auto f = [&](SvrCon &con)
	{
		ExternalSvrCon *pCon = dynamic_cast<ExternalSvrCon *>(&con);
		L_COND(pCon);
		if (pCon->IsVerify())
		{
			MsgNtfCreateSession ntf;
			ntf.cid = pCon->GetId();
			ntf.uin = pCon->GetUin();
			ntf.addr = pCon->GetRemoteAddr();
			L_DEBUG("CMD_NTF_CREATE_SESSION uin=%lld", ntf.uin);
			this->Send(CMD_NTF_CREATE_SESSION, ntf);
		}
	};
	Server::Ins().m_client_listener.GetConnMgr().Foreach(f);
}

bool InnerSvrCon::RegSvrId(uint16 id)
{
	L_COND_F(0 != id);
	if (0 != m_svr_id)
	{
		L_ERROR("repeated reg svr id. m_svr_id=%d", m_svr_id);
		return false;
	}
	if (!RegSvrMgr::Ins().Insert(id, this))
	{
		L_WARN("repeated reg svr id=%d", id);
		return false;
	}
	m_svr_id = id;

	

	return true;
}

void InnerSvrCon::SetVerifySvr()
{
	L_COND(VerifySvrMgr::Ins().InsertVerify(this));
	m_is_verify = true;
}

bool InnerSvrCon::Send(const acc::ASMsg &as_data)
{
	std::string tcp_pack;
	L_COND_F(as_data.Serialize(tcp_pack));
	return SendPack(tcp_pack.c_str(), tcp_pack.length());
}

void InnerSvrCon::OnRecv(const lc::MsgPack &msg)
{
	ASMsg as_data;
	L_COND(as_data.Parse(msg.data, msg.len));
//	L_DEBUG("as_data.cmd=%d as_data.msg_len=%d tcp_pack.length=%d", as_data.cmd, as_data.msg_len, msg.len);
	if (CMD_REQ_FORWARD == as_data.cmd)//直接转发，不用分发，快点
	{
		MsgForward f_msg;
		L_COND(f_msg.Parse(as_data.msg, as_data.msg_len));
		if (0 == f_msg.cid)
		{
			L_WARN("CMD_REQ_FORWARD 0 == f_msg.cid");
			return;
		}
		ExternalSvrCon *pClient = Server::Ins().FindClientSvrCon(f_msg.cid);
		if (nullptr == pClient)
		{
			L_DEBUG("find cid fail. cid=%lld", f_msg.cid);
			return;
		}
		if (!pClient->IsVerify())
		{
			L_ERROR("client is not verify. cid=%lld", f_msg.cid);
			return;
		}
		pClient->SendMsg(f_msg.cmd, f_msg.msg, f_msg.msg_len);
	}
	else
	{
		SvrCtrlMsgDispatch::Ins().DispatchMsg(*this, as_data);
	}
}


RegSvrMgr::RegSvrMgr()
{

}


bool RegSvrMgr::Insert(uint16 svr_id, InnerSvrCon *pSvr)
{
	L_COND_F(0 != svr_id);
	L_COND_F(nullptr != pSvr);

	auto it = m_id_2_svr.find(svr_id);
	if (it != m_id_2_svr.end())
	{
		L_ERROR("repeated reg svr id. %d", svr_id);
		return false;
	}
	m_id_2_svr.insert(make_pair(svr_id, pSvr));
	return true;
}

bool RegSvrMgr::Remove(uint16 svr_id)
{
	L_COND_F(0 != svr_id);
	size_t i = m_id_2_svr.erase(svr_id);
	return i == 1;
}

InnerSvrCon * RegSvrMgr::Find(uint16 svr_id)
{
	L_COND_R(0 != svr_id, nullptr);
	auto it = m_id_2_svr.find(svr_id);
	if (it == m_id_2_svr.end())
	{
		return nullptr;
	}
	L_COND_R(nullptr != it->second, nullptr);
	return it->second;
}

bool VerifySvrMgr::InsertVerify(InnerSvrCon *pSvr)
{
	L_COND_F(nullptr != pSvr);
	if (!m_verify_svr.insert(pSvr).second)
	{
		L_ERROR("repeated insert verify svr. SvrId=%d", pSvr->GetSvrId());
		return false;
	}
	return true;
}

bool VerifySvrMgr::RemoveVerify(InnerSvrCon *pSvr)
{
	L_COND_F(nullptr != pSvr);
	size_t i = m_verify_svr.erase(pSvr);
	return i == 1;
}

VerifySvrMgr::VerifySvrMgr()
	:m_bl_idx(0)
{

}

InnerSvrCon * VerifySvrMgr::GetBLVerifySvr()
{
	if (m_verify_svr.empty())
	{
		return nullptr;
	}

	m_bl_idx++;
	if (m_bl_idx >= m_verify_svr.size())
	{
		m_bl_idx = 0;
	}
	uint32 idx = 0;
	for (auto &v : m_verify_svr)
	{
		if (m_bl_idx == idx)
		{
			return v;
		}
		idx++;
	}
	L_COND_R(false, nullptr);
}