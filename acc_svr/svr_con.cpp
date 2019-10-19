#include "svr_con.h"
#include <utility>
#include "client_con.h"
#include "com.h"
#include "server.h"

using namespace std;
using namespace acc;
using namespace lc;


namespace
{
	void Parse_CMD_REQ_DISCON_ALL(SvrSvrCon &con, const acc::ASMsg &msg)
	{
		L_INFO("svr req disconnect all client");
		BaseConMgr &conMgr = Server::Obj().m_client_listener.GetConnMgr();
		auto f=[](SvrCon &svr_con)
		{
			svr_con.DisConnect();
		};
		conMgr.Foreach(f);
	}



	void Parse_CMD_REQ_DISCON(SvrSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqDiscon req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");

		ClientSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid);
		L_COND(pClient);
		pClient->DisConnect();
	}

	void Parse_CMD_REQ_REG(SvrSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqReg req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");
		MsgRspReg rsp;
		rsp.svr_id = req.svr_id;
		if (!con.RegSvrId(req.svr_id))
		{
			L_WARN("ignore repeated reg svr id=%d", req.svr_id);
			rsp.svr_id = 0;
		}
		if (req.is_verify)
		{
			con.SetVerifySvr();
		}
		con.Send(CMD_RSP_REG, rsp);
	}

	void Parse_CMD_REQ_VERIFY_RET(SvrSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqVerifyRet req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");

		ClientSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid);
		L_COND(pClient);

		pClient->SetVerify(req.is_success);

	}

	void Parse_CMD_REQ_SET_MAIN_CMD(SvrSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqSetMainCmd req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");

		ClientSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid);
		L_COND(pClient);
		pClient->SetMainCmd2SvrId(req.main_cmd, req.svr_id);

	}

	void Parse_CMD_REQ_BROADCAST(SvrSvrCon &con, const acc::ASMsg &msg)
	{
		MsgReqBroadCast req;
		L_COND(req.Parse(msg.msg, msg.msg_len), "parse ctrl msg fail");
		L_COND(req.cid_len <= acc::MAX_BROADCAST_CID_NUM);

		string tcp_pack;
		tcp_pack.append((char *)&req.cmd, sizeof(req.cmd));
		tcp_pack.append(req.msg, req.msg_len);
		if (req.cid_len == 0)
		{
			auto f = [&req, &tcp_pack](SvrCon &con)
			{
				ClientSvrCon *pClient = dynamic_cast<ClientSvrCon *>(&con);
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
				ClientSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid_s[i]);
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
	m_cmd_2_handle[CMD_REQ_REG] = Parse_CMD_REQ_REG;
	m_cmd_2_handle[CMD_REQ_VERIFY_RET] = Parse_CMD_REQ_VERIFY_RET;
	m_cmd_2_handle[CMD_REQ_BROADCAST] = Parse_CMD_REQ_BROADCAST;
	m_cmd_2_handle[CMD_REQ_SET_MAIN_CMD] = Parse_CMD_REQ_SET_MAIN_CMD;
	m_cmd_2_handle[CMD_REQ_DISCON] = Parse_CMD_REQ_DISCON;
	m_cmd_2_handle[CMD_REQ_DISCON_ALL] = Parse_CMD_REQ_DISCON_ALL;
}

void SvrCtrlMsgDispatch::DispatchMsg(SvrSvrCon &con, const acc::ASMsg &msg)
{
	auto handle_it = m_cmd_2_handle.find((acc::Cmd)msg.cmd);
	if (handle_it == m_cmd_2_handle.end())
	{
		L_ERROR("find cmd handler fail. ctrl_cmd=%d", msg.cmd);
		return;
	}

	(*handle_it->second)(con, msg);
}

SvrSvrCon::SvrSvrCon()
{

}

SvrSvrCon::~SvrSvrCon()
{
	if (0 != m_svr_id)
	{
		if (!SvrRegMgr::Obj().Remove(m_svr_id))
		{
			L_ERROR("SvrRegMgr::Obj().Remove fail. m_svr_id=%d", m_svr_id);
		}
		m_svr_id = 0;
	}
	if (m_is_verify)
	{
		if (!SvrRegMgr::Obj().RemoveVerify(this))
		{
			L_ERROR("SvrRegMgr::Obj().RemoveVerify fail.");
		}
		m_is_verify = false;
	}
}

bool SvrSvrCon::RegSvrId(uint16 id)
{
	L_COND_F(0 != id);
	if (0 != m_svr_id)
	{
		L_ERROR("repeated reg svr id. m_svr_id=%d", m_svr_id);
		return false;
	}
	L_COND_F(SvrRegMgr::Obj().Insert(id, this));
	m_svr_id = id;
	return true;
}

void SvrSvrCon::SetVerifySvr()
{
	L_COND(SvrRegMgr::Obj().InsertVerify(this));
	m_is_verify = true;
}

bool SvrSvrCon::Send(const acc::ASMsg &as_data)
{
	std::string tcp_pack;
	COND_F(as_data.Serialize(tcp_pack));
	return SendData(tcp_pack.c_str(), tcp_pack.length());
}

void SvrSvrCon::OnRecv(const lc::MsgPack &msg)
{
	ASMsg as_data;
	L_COND(as_data.Parse(msg.data, msg.len));
	if (CMD_REQ_FORWARD == as_data.cmd)//直接转发，不用分发，快点
	{
		ForwardMsg f_msg;
		L_COND(f_msg.Parse(as_data.msg, as_data.msg_len));

		ClientSvrCon *pClient = Server::Obj().FindClientSvrCon(f_msg.cid);
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

SvrRegMgr::SvrRegMgr()
	:m_bl_idx(0)
{

}

SvrSvrCon * SvrRegMgr::GetBLVerifySvr()
{
	if (m_verify_svr.empty())
	{
		return nullptr;
	}

	m_bl_idx++;
	if (m_bl_idx>=m_verify_svr.size())
	{
		m_bl_idx = 0;
	}
	uint32 idx = 0;
	for (auto &v : m_verify_svr)
	{
		if (m_bl_idx==idx)
		{
			return v;
		}
		idx++;
	}
	L_COND_R(false, nullptr);
}

bool SvrRegMgr::Insert(uint16 svr_id, SvrSvrCon *pSvr)
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

bool SvrRegMgr::Remove(uint16 svr_id)
{
	L_COND_F(0 != svr_id);
	size_t i = m_id_2_svr.erase(svr_id);
	L_COND_F(i == 1);
	return true;
}

SvrSvrCon * SvrRegMgr::Find(uint16 svr_id)
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

bool SvrRegMgr::InsertVerify(SvrSvrCon *pSvr)
{
	L_COND_F(nullptr != pSvr);
	if (!m_verify_svr.insert(pSvr).second)
	{
		L_ERROR("repeated insert verify svr. SvrId=%d", pSvr->GetSvrId());
		return false;
	}
	return true;
}

bool SvrRegMgr::RemoveVerify(SvrSvrCon *pSvr)
{
	L_COND_F(nullptr != pSvr);
	size_t i = m_verify_svr.erase(pSvr);
	L_COND_F(i == 1);
	return true;
}
