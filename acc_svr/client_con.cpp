#include "client_con.h"
#include "svr_con.h"
#include "com.h"

using namespace std;
using namespace acc;
using namespace lc;

ClientSvrCon::ClientSvrCon()
	:m_state(State::INIT)
	,m_uin(0)
{
}

ClientSvrCon::~ClientSvrCon()
{

}

bool ClientSvrCon::SetVerify()
{
	L_COND_F(m_state == State::WAIT_VERIFY);
	m_state = State::VERIFYED;
	return true;
}

void ClientSvrCon::OnRecv(const lc::MsgPack &msg)
{
	switch (m_state)
	{
	default:
		L_ERROR("unknow state %d", (int)m_state);
		break;
	case ClientSvrCon::State::INIT:
		Forward2VerifySvr(msg);
		break;
	case ClientSvrCon::State::WAIT_VERIFY:
		L_WARN("client repeated req verify");
		DisConnect();
		break;
	case ClientSvrCon::State::VERIFYED:
		Forward2Svr(msg);
		break;
	}
	return;
}

void ClientSvrCon::Forward2VerifySvr(const lc::MsgPack &msg)
{
	L_COND(State::INIT == m_state);

	ASData as_data;
	{//client tcp pack to ASData
		as_data.is_ctrl = 0;
		auto &forward_msg = as_data.union_msg.forward_msg;
		if (msg.len < sizeof(forward_msg.cmd))
		{
			L_WARN("too short client msg len. msg.len=%d", msg.len);
			return;
		}
		forward_msg.cid = GetId();
		const char *cur = msg.data;
		forward_msg.cmd = (decltype(forward_msg.cmd))(*cur);
		cur = cur + sizeof(forward_msg.cmd);
		forward_msg.msg = cur;
		forward_msg.msg_len = msg.len - sizeof(forward_msg.cmd);
	}

	SvrSvrCon *pSvr = SvrRegMgr::Obj().GetBLVerifySvr();
	if (nullptr == pSvr)
	{
		L_WARN("can't find verfify svr. maybe you havn't reg your verify svr.");
		return;
	}

	pSvr->Send(as_data);
	m_state = State::WAIT_VERIFY;
}

void ClientSvrCon::Forward2Svr(const lc::MsgPack &msg) const
{

}
