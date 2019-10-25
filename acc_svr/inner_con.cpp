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
	void Parse_CMD_REQ_SET_HEARTBEAT_INFO(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqSetHeartbeatInfo req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		if (0 == req.cmd || 0 == req.rsp_cmd || 0 == req.interval_sec)
		{
			L_WARN("rev wrong MsgReqSetHeartbeatInfo, ignore");
			return;
		}

		HeartbeatInfo::Obj().cmd = req.cmd;
		HeartbeatInfo::Obj().rsp_cmd = req.rsp_cmd;
		HeartbeatInfo::Obj().interval_sec = req.interval_sec;
	}

	void Parse_CMD_REQ_DISCON_ALL(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		L_INFO("svr req disconnect all client");
		BaseConMgr &conMgr = Server::Obj().m_client_listener.GetConnMgr();
		auto f=[](SvrCon &svr_con)
		{
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
		ExternalSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid);
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
	}

	void Parse_CMD_REQ_VERIFY_RET(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqVerifyRet req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		if (req.cid == 0)
		{
			L_WARN("CMD_REQ_VERIFY_RET cid==0");
			return;
		}

		ExternalSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid);
		L_COND(pClient);
		if (pClient->IsVerify())
		{
			L_DEBUG("repeated verify"); //客户端重复请求验证，重复接收验证成功。
			return;
		}

		pClient->SetVerify(req.is_success);

		//通知创建会话
		MsgNtfCreateSession ntf;
		ntf.cid = req.cid;
		auto f = [&ntf](SvrCon &con)
		{
			InnerSvrCon *pCon = dynamic_cast<InnerSvrCon *>(&con);
			L_COND(pCon);
			if (0 != pCon->GetSvrId())
			{
				pCon->Send(CMD_NTF_CREATE_SESSION, ntf);
			}
		};
		Server::Obj().m_svr_listener.GetConnMgr().Foreach(f);

	}

	void Parse_CMD_REQ_SET_MAIN_CMD_2_SVR(InnerSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqSetMainCmd2Svr req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		MsgRspSetMainCmd2Svr rsp;
		rsp.cid = req.cid;
		rsp.main_cmd = req.main_cmd;
		rsp.svr_id = req.svr_id;
		if (req.cid == 0)
		{
			L_WARN("CMD_REQ_VERIFY_RET cid==0");
			rsp.svr_id = 0;
			con.Send(CMD_RSP_SET_MAIN_CMD_2_SVR, rsp);
			return;
		}

		ExternalSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid);
		if (nullptr == pClient)
		{
			rsp.svr_id = 0;
			con.Send(CMD_RSP_SET_MAIN_CMD_2_SVR, rsp);
			L_DEBUG("client not exist");
			return;
		}

		pClient->SetMainCmd2SvrId(req.main_cmd, req.svr_id);
		con.Send(CMD_RSP_SET_MAIN_CMD_2_SVR, rsp);
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
		tcp_pack.append((char *)&req.cmd, sizeof(req.cmd));
		tcp_pack.append(req.msg, req.msg_len);
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
			Server::Obj().m_client_listener.GetConnMgr().Foreach(f);
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
				ExternalSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid_s[i]);
				if (nullptr == pClient)//客户端刚刚断开，svr还没收到消息，正常。
				{
					continue;
				}
				if (pClient->IsVerify())
				{
					L_ERROR("unverifyed client can't be set uin");//哪里错误了，未认证不能设置uin
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
	m_cmd_2_handle[CMD_REQ_DISCON]             = Parse_CMD_REQ_DISCON;
	m_cmd_2_handle[CMD_REQ_DISCON_ALL]         = Parse_CMD_REQ_DISCON_ALL;
	m_cmd_2_handle[CMD_REQ_SET_HEARTBEAT_INFO] = Parse_CMD_REQ_SET_HEARTBEAT_INFO;
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
}

InnerSvrCon::~InnerSvrCon()
{
	if (0 != m_svr_id)
	{
		if (!RegSvrMgr::Obj().Remove(m_svr_id))
		{
			L_ERROR("SvrRegMgr::Obj().Remove fail. m_svr_id=%d", m_svr_id);
		}
		m_svr_id = 0;
	}
	if (m_is_verify)
	{
		if (!VerifySvrMgr::Obj().RemoveVerify(this))
		{
			L_ERROR("SvrRegMgr::Obj().RemoveVerify fail.");
		}
		m_is_verify = false;
	}
}

bool InnerSvrCon::RegSvrId(uint16 id)
{
	L_COND_F(0 != id);
	if (0 != m_svr_id)
	{
		L_ERROR("repeated reg svr id. m_svr_id=%d", m_svr_id);
		return false;
	}
	if (!RegSvrMgr::Obj().Insert(id, this))
	{
		L_WARN("repeated reg svr id=%d", id);
		return false;
	}
	m_svr_id = id;

	//通知创建会话
	{
		MsgNtfCreateSession ntf;
		auto f = [&](SvrCon &con)
		{
			ExternalSvrCon *pCon = dynamic_cast<ExternalSvrCon *>(&con);
			L_COND(pCon);
			if (pCon->IsVerify())
			{
				ntf.cid = id;
				this->Send(CMD_NTF_CREATE_SESSION, ntf);
			}
		};
		Server::Obj().m_client_listener.GetConnMgr().Foreach(f);
	}

	return true;
}

void InnerSvrCon::SetVerifySvr()
{
	L_COND(VerifySvrMgr::Obj().InsertVerify(this));
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
	if (CMD_REQ_FORWARD == as_data.cmd)//直接转发，不用分发，快点
	{
		MsgForward f_msg;
		L_COND(f_msg.Parse(as_data.msg, as_data.msg_len));
		if (0 == f_msg.cid)
		{
			L_WARN("CMD_REQ_FORWARD 0 == f_msg.cid");
			return;
		}
		ExternalSvrCon *pClient = Server::Obj().FindClientSvrCon(f_msg.cid);
		if (nullptr == pClient)
		{
			L_DEBUG("find cid fail. cid=%lld", f_msg.cid);
			return;
		}

		pClient->SendMsg(f_msg.cmd, f_msg.msg, f_msg.msg_len);
	}
	else
	{
		SvrCtrlMsgDispatch::Obj().DispatchMsg(*this, as_data);
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