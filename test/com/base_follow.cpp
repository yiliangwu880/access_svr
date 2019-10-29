#include "base_follow.h"
#include "cfg.h"

using namespace std;
using namespace lc;
using namespace su;
using namespace acc;

namespace
{
	static const uint32 CMD_VERIFY = ((uint32)BF_SVR1 << 16) | 1;
	static const uint32 CMD_MSG = ((uint32)BF_SVR1 << 16) | 2;
	static const uint32 CMD_BEAT_REQ = 3;
	static const uint32 CMD_BEAT_RSP = 4;
}

BaseFlowClient::BaseFlowClient(BaseFunFollowMgr &mgr)
	:m_mgr(mgr)
{
	m_state = State::WAIT_SVR_REG;
}

void BaseFlowClient::SendVerify()
{
	UNIT_ASSERT(IsConnect());
	UNIT_ASSERT(State::WAIT_SVR_REG == m_state);
	SendStr(CMD_VERIFY, "verify");
	m_state = State::WAIT_VERFIY_RSP;
}

void BaseFlowClient::OnRecv(const lc::MsgPack &msg)
{
	UNIT_ASSERT(msg.len >= sizeof(uint32));
	string str;
	uint32 cmd = 0;
	{
		cmd = *((const uint32 *)msg.data);
		const char *msg_str = msg.data + sizeof(uint32);
		uint32 msg_len = msg.len - sizeof(uint32);
		str.append(msg_str, msg_len);
	}
	if (CMD_VERIFY == cmd) //verify rsp
	{
		UNIT_ASSERT(State::WAIT_VERFIY_RSP == m_state);
		UNIT_ASSERT(string("verify_ok") == str);
		SendStr(CMD_MSG, "msg");
		m_state = State::WAIT_SVR_RSP;
		return;
	}
	else if (CMD_MSG == cmd)
	{
		UNIT_ASSERT(State::WAIT_SVR_RSP == m_state);
		UNIT_ASSERT(string("msg") == str);
		DisConnect();
		m_state = State::END;
	}
	else
	{
		UNIT_ERROR("unknow cmd=%d", cmd);
		UNIT_ASSERT(false);
	}
}

void BaseFlowClient::OnConnected()
{

}

void BaseFlowClient::OnDisconnected()
{

}



BaseFlowSvr::BaseFlowSvr(BaseFunFollowMgr &mgr)
	:m_mgr(mgr)
{
	m_state = State::WAIT_SVR_REG;
}

void BaseFlowSvr::OnRegResult(uint16 svr_id)
{
	UNIT_ASSERT(State::WAIT_SVR_REG == m_state);
	UNIT_ASSERT(BF_SVR1 == svr_id);
	m_state = State::WAIT_CLIENT_REQ_VERFIY;
	m_mgr.m_client.SendVerify();
	UNIT_INFO("reg ok");
}

void BaseFlowSvr::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	UNIT_ASSERT(id.cid != 0);
	UNIT_ASSERT(id.acc_id == 0);
	UNIT_INFO("cmd=%x", cmd);
	UNIT_ASSERT(CMD_VERIFY == cmd);
	UNIT_ASSERT(State::WAIT_CLIENT_REQ_VERFIY == m_state);
	string s(msg, msg_len);
	UNIT_ASSERT(s == "verify");
	string rsp_msg = "verify_ok";
	AllADFacadeMgr::Obj().ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
	m_state = State::WAIT_CLIENT_MSG;

}

void BaseFlowSvr::OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	UNIT_ASSERT(CMD_MSG == cmd);
	UNIT_ASSERT(State::WAIT_CLIENT_MSG == m_state);
	string s(msg, msg_len);
	UNIT_ASSERT(s == "msg");
	AllADFacadeMgr::Obj().SendToClient(id, cmd, msg, msg_len);
	m_state = State::WAIT_CLIENT_DISCON;
}

void BaseFlowSvr::OnClientDisCon(const SessionId &id)
{
	UNIT_ASSERT(id.cid != 0);
	UNIT_ASSERT(id.acc_id == 0);
	UNIT_ASSERT(State::WAIT_CLIENT_DISCON == m_state);
	m_state = State::END;

	UNIT_ASSERT(BaseFlowClient::State::END == m_mgr.m_client.m_state);
	UNIT_INFO("BaseFlowSvr end");
	m_mgr.StartHeartbeatTest();
}

void BaseFlowSvr::OnClientConnect(const SessionId &id)
{
	UNIT_ASSERT(State::WAIT_CLIENT_MSG == m_state);
}

void BaseFlowSvr::OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id)
{

}


BaseFunFollowMgr::BaseFunFollowMgr()
	:m_client(*this)
	,m_svr(*this)
	,m_h_client(*this)
	,m_h_svr(*this)
{
	m_state = State::RUN_CORRECT_FOLLOW;
}


bool BaseFunFollowMgr::Init()
{
	const std::vector<Addr> &inner_vec = CfgMgr::Obj().m_inner_vec;
	const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
	UNIT_ASSERT(inner_vec.size() > 1);
	UNIT_ASSERT(ex_vec.size() > 1);
	Addr ex_addr = ex_vec[0];
	auto vec = inner_vec;
	vec.resize(1);
	UNIT_ASSERT(AllADFacadeMgr::Obj().Init(vec, 1, true));
	AllADFacadeMgr::Obj().m_svr_cb = &m_svr;
	UNIT_ASSERT(m_client.ConnectInit(ex_addr.ip.c_str(), ex_addr.port));
	return true;

}

void BaseFunFollowMgr::StartHeartbeatTest()
{
	UNIT_INFO("start heartbeat test");
	UNIT_ASSERT(m_state == State::RUN_CORRECT_FOLLOW);
	m_state = State::RUN_HEARBEAT;
	// svr连接对象还是复用 BaseFlowSvr创建的
	AllADFacadeMgr::Obj().m_svr_cb = &m_h_svr;
	m_h_svr.Start();

	//delay start client
	auto f = [&]()
	{
		const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
		UNIT_ASSERT(ex_vec.size() > 1);
		UNIT_ASSERT(m_h_client.ConnectInit(ex_vec[0].ip.c_str(), ex_vec[0].port));
		UNIT_INFO("delay 1sec, start m_h_client connect");
	};
	m_tm.StopTimer();
	m_tm.StartTimer(1*1000, f);
}

void BaseFunFollowMgr::CheckBearHeatEnd()
{
	//验收再测试状态，因为client, svr,是网络消息接收才改变最后状态，不同步
	auto f = [&]()
	{
		UNIT_ASSERT(m_h_svr.m_state == HearBeatSvr::State::END);
		UNIT_ASSERT(m_h_client.m_state == HearBeatClient::State::END);
		UNIT_INFO("BaseFunFollowMgr::CheckBearHeatEnd");
	};
	m_tm.StopTimer();
	m_tm.StartTimer(1 * 1000, f);
}

void BaseClient::SendStr(uint32 cmd, const std::string &msg)
{
	string pack;
	pack.append((const char *)&cmd, sizeof(cmd));
	pack.append(msg);
	SendPack(pack);
}

void BaseClient::OnRecv(const lc::MsgPack &msg)
{
	UNIT_ASSERT(msg.len >= sizeof(uint32));
	string str;
	uint32 cmd = 0;
	{
		cmd = *((const uint32 *)msg.data);
		const char *msg_str = msg.data + sizeof(uint32);
		uint32 msg_len = msg.len - sizeof(uint32);
		str.append(msg_str, msg_len);
	}
	OnRecvMsg(cmd, str);
}

HearBeatClient::HearBeatClient(BaseFunFollowMgr &mgr)
	:m_mgr(mgr)
{
	m_state = State::WAIT_VERFIY_RSP;
}

void HearBeatClient::OnRecvMsg(uint32 cmd, const std::string &msg)
{
	if (CMD_VERIFY == cmd)
	{
		UNIT_ASSERT(m_state == State::WAIT_VERFIY_RSP);
		UNIT_INFO("HearBeatClient verify ok, send beat");
		SendStr(CMD_BEAT_REQ, "");
		m_state = State::WAIT_BEAT_RSP;
	}
	else if (CMD_BEAT_RSP == cmd)
	{
		UNIT_INFO("rev beat rsp");
		UNIT_ASSERT(m_state == State::WAIT_BEAT_RSP);
		m_state = State::WAIT_DISCONNECTED;
	}
	else
	{
		UNIT_INFO("unknow cmd =%x", cmd);
		UNIT_ASSERT(false);
	}
}


void HearBeatClient::OnConnected()
{
	UNIT_ASSERT(m_state == State::WAIT_VERFIY_RSP);
	SendStr(CMD_VERIFY, "verify");
	UNIT_INFO("client send verify req");
}

void HearBeatClient::OnDisconnected()
{
	UNIT_ASSERT(m_state == State::WAIT_DISCONNECTED);
	m_state = State::END;
	UNIT_INFO("client beat timeout, disconnted");
}

HearBeatSvr::HearBeatSvr(BaseFunFollowMgr &mgr)
	: m_mgr(mgr)
{
	m_state = State::WAIT_VERFIY_REQ;
}

void HearBeatSvr::Start()
{
	UNIT_ASSERT(m_state == State::WAIT_VERFIY_REQ);
	UNIT_INFO(" HearBeatSvr::Start");
	AllADFacadeMgr::Obj().SetHeartbeatInfo(CMD_BEAT_REQ, CMD_BEAT_RSP, 2);
}

void HearBeatSvr::OnRegResult(uint16 svr_id)
{

}

void HearBeatSvr::OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{

}

void HearBeatSvr::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	UNIT_ASSERT(m_state == State::WAIT_VERFIY_REQ);
	UNIT_INFO("svr rev verify req cmd=%x", cmd);
	UNIT_ASSERT(CMD_VERIFY == cmd);
	string s(msg, msg_len);
	UNIT_ASSERT(s == "verify");
	string rsp_msg = "verify_ok";
	AllADFacadeMgr::Obj().ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
	m_state = State::WAIT_CLIENT_BEAT_TIME_OUT;
}

void HearBeatSvr::OnClientDisCon(const SessionId &id)
{
	UNIT_ASSERT(m_state == State::WAIT_CLIENT_BEAT_TIME_OUT);
	m_state = State::END;
	UNIT_INFO("hearbeat svr end");
	m_mgr.CheckBearHeatEnd();
}

void HearBeatSvr::OnClientConnect(const SessionId &id)
{

}

void HearBeatSvr::OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id)
{

}
