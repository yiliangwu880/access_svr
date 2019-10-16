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
	void Parse_CMD_REQ_REG(SvrSvrCon &con, const acc::AccSvrMsg &msg)
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

	void Parse_CMD_REQ_VERIFY_RET(SvrSvrCon &con, const acc::AccSvrMsg &msg)
	{
		MsgReqVerifyRet req;
		bool ret = CtrlMsgProto::Parse(msg, req);
		L_COND(ret, "parse ctrl msg fail");

		ClientSvrCon *pClient = Server::Obj().FindClientSvrCon(req.cid);
		L_COND(pClient);

		L_COND(pClient->SetVerify());
	}

}
void SvrCtrlMsgDispatch::Init()
{
	m_cmd_2_handle[CMD_REQ_REG] = Parse_CMD_REQ_REG;
	m_cmd_2_handle[CMD_REQ_VERIFY_RET] = Parse_CMD_REQ_VERIFY_RET;
}

void SvrCtrlMsgDispatch::DispatchMsg(SvrSvrCon &con, const acc::AccSvrMsg &msg)
{
	auto handle_it = m_cmd_2_handle.find((acc::Cmd)msg.cmd);
	if (handle_it == m_cmd_2_handle.end())
	{
		L_ERROR("find cmd handler fail. ctrl_cmd=%d", msg.cmd);
		return;
	}

	(*handle_it->second)(con, msg);
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

bool SvrSvrCon::Send(const acc::ASData &as_data)
{
	lc::MsgPack msg_pack;
	COND_F(Com::ASData2MsgPack(as_data, msg_pack));
	return SendData(msg_pack);
}

void SvrSvrCon::OnRecv(const lc::MsgPack &msg)
{
	ASData as_data;
	L_COND(as_data.Parse(msg.data, msg.len));
	if (1 == as_data.is_ctrl)
	{
		SvrCtrlMsgDispatch::Obj().DispatchMsg(*this, as_data.union_msg.ctrl);
	}
	else if(0 == as_data.is_ctrl)
	{
		auto &forward_msg = as_data.union_msg.forward_msg;
		ClientSvrCon *pClient = Server::Obj().FindClientSvrCon(forward_msg.cid);
		if(nullptr == pClient)
		{
			L_DEBUG("find cid fail. cid=%lld", forward_msg.cid);
			return;
		}
		MsgPack msg_pack;
		L_COND(Com::ASData2MsgPack(as_data, msg_pack));
		pClient->SendData(msg_pack);
	}
	else
	{
		L_ERROR("unknow is_ctrl =%d", as_data.is_ctrl);
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
