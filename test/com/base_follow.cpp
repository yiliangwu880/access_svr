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
	static const uint32 CMD_BROADCAST = 5;
	static const uint32 CMD_BROADCAST_PART = 6;
	static const uint32 CMD_SVR1_MSG = ((uint32)BF_SVR1 << 16) | 10;
	static const uint32 CMD_SVR2_MSG = ((uint32)BF_SVR2 << 16) | 10;
	static const uint32 CMD_SVR3_MSG = ((uint32)BF_SVR3 << 16) | 10;
}

BaseFlowClient::BaseFlowClient(BaseFunTestMgr &mgr)
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



BaseFlowSvr::BaseFlowSvr(BaseFunTestMgr &mgr)
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
	m_mgr.m_svr1.ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
	m_state = State::WAIT_CLIENT_MSG;

}

void BaseFlowSvr::OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	UNIT_ASSERT(CMD_MSG == cmd);
	UNIT_ASSERT(State::WAIT_CLIENT_MSG == m_state);
	string s(msg, msg_len);
	UNIT_ASSERT(s == "msg");
	m_mgr.m_svr1.SendToClient(id, cmd, msg, msg_len);
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


BaseFunTestMgr::BaseFunTestMgr()
	:m_client(*this)
	,m_svr(*this)
	,m_h_client(*this)
	,m_h_svr(*this)
	,m_bd_client({*this,*this,*this})
	,m_bd_svr(*this)
	,m_route_svr1(*this)
	, m_route_svr2(*this)
	, m_route_client(*this)

{
	m_state = State::RUN_CORRECT_FOLLOW;
	m_route_svr1.m_svr_id = 1;
	m_route_svr2.m_svr_id = 2;
}


bool BaseFunTestMgr::Init()
{
	const std::vector<Addr> &inner_vec = CfgMgr::Obj().m_inner_vec;
	const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
	UNIT_ASSERT(inner_vec.size() > 1);
	UNIT_ASSERT(ex_vec.size() > 1);
	Addr ex_addr = ex_vec[0];
	auto vec = inner_vec;
	vec.resize(1);
	UNIT_ASSERT(m_svr1.Init(vec, 1, true));
	m_svr1.m_svr_cb = &m_svr;

	UNIT_ASSERT(m_svr2.Init(vec, 2));
	m_svr2.m_svr_cb = &m_route_svr2;
	UNIT_ASSERT(m_client.ConnectInit(ex_addr.ip.c_str(), ex_addr.port));
	//{
	//	UNIT_INFO("tmp test");
	//	StartCheckRoute();
	//}
	return true;

}

void BaseFunTestMgr::StartHeartbeatTest()
{
	UNIT_INFO("start heartbeat test");
	UNIT_ASSERT(m_state == State::RUN_CORRECT_FOLLOW);
	m_state = State::RUN_HEARBEAT;
	// svr连接对象还是复用 BaseFlowSvr创建的
	m_svr1.m_svr_cb = &m_h_svr;
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

void BaseFunTestMgr::CheckBearHeatEnd()
{
	//验收再测试状态，因为client, svr,是网络消息接收才改变最后状态，不同步
	auto f = [&]()
	{
		UNIT_ASSERT(m_state == State::RUN_HEARBEAT);
		UNIT_ASSERT(m_h_svr.m_state == HearBeatSvr::State::END);
		UNIT_ASSERT(m_h_client.m_state == HearBeatClient::State::END);
		UNIT_INFO("BaseFunFollowMgr::CheckBearHeatEnd");
		StartCheckBD();
	};
	m_tm.StopTimer();
	m_tm.StartTimer(1 * 1000, f);
}

void BaseFunTestMgr::StartCheckBD()
{
	m_state = State::RUN_BROADCAST_DISCON;
	m_svr1.m_svr_cb = &m_bd_svr;
	m_bd_svr.Start();
}

void BaseFunTestMgr::StartCheckRoute()
{
	m_state = State::RUN_ROUTE;
	UNIT_INFO("start RUN_ROUTE");
	m_route_client.Start();
}

void BaseFunTestMgr::End()
{
	UNIT_INFO("--------------------base test end--------------------");
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

HearBeatClient::HearBeatClient(BaseFunTestMgr &mgr)
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

HearBeatSvr::HearBeatSvr(BaseFunTestMgr &mgr)
	: m_mgr(mgr)
{
	m_state = State::WAIT_VERFIY_REQ;
}

void HearBeatSvr::Start()
{
	UNIT_ASSERT(m_state == State::WAIT_VERFIY_REQ);
	UNIT_INFO(" HearBeatSvr::Start");
	m_mgr.m_svr1.SetHeartbeatInfo(CMD_BEAT_REQ, CMD_BEAT_RSP, 2);
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
	m_mgr.m_svr1.ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
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

BDClient::BDClient(BaseFunTestMgr &mgr)
	:m_mgr(mgr)
{
	m_id = 0;
}

void BDClient::OnRecvMsg(uint32 cmd, const std::string &msg)
{
	if (CMD_VERIFY == cmd)
	{
		UNIT_INFO("verify ok");
	}
	else if (CMD_BROADCAST == cmd)
	{
		UNIT_ASSERT(msg == "CMD_BROADCAST");
		m_mgr.m_bd_svr.ClientRevBroadCmd();
	}
	else if (CMD_BROADCAST_PART == cmd)
	{
		UNIT_ASSERT(msg == "CMD_BROADCAST");
		m_mgr.m_bd_svr.ClientRevBroadPartCmd();
	}
	else
	{
		UNIT_INFO("unknow cmd =%x", cmd);
		UNIT_ASSERT(false);
	}
}

void BDClient::OnConnected()
{
	m_mgr.m_bd_svr.ClientOnConnected(m_id);
	SendStr(CMD_VERIFY, "verify");
}

void BDClient::OnDisconnected()
{
	UNIT_INFO("diconnect BDClient. idx=%d", m_id);
}

BDSvr::BDSvr(BaseFunTestMgr &mgr)
	: m_mgr(mgr)
{
	m_state = State::WAIT_ALL_CLIENT_CONNECT;
	m_broadpartCmd_cnt = 0;
	m_broadCmd_cnt = 0;
	m_tmp_num = 0;
}

void BDSvr::Start()
{
	UNIT_ASSERT(m_state == State::WAIT_ALL_CLIENT_CONNECT);
	UNIT_INFO("test broadcast, disconnect start");

	//all client connect init
	const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
	UNIT_ASSERT(ex_vec.size() > 1);
	uint32 idx = 0;
	for (auto &client : m_mgr.m_bd_client)
	{
		client.m_id = idx;
		UNIT_ASSERT(client.ConnectInit(ex_vec[0].ip.c_str(), ex_vec[0].port));
		idx++;
	}
	m_client_set.clear();
}

void BDSvr::ClientOnConnected(uint32 idx)
{
	UNIT_ASSERT(m_state == State::WAIT_ALL_CLIENT_CONNECT);
	bool r = m_client_set.insert(idx).second;
	UNIT_ASSERT(r);
	if (m_client_set.size() == m_mgr.m_bd_client.size())
	{//all connect
		m_state = State::WAIT_ALL_VERIFY_OK;
		m_client_set.clear();
	}
}

void BDSvr::ClientRevBroadCmd()
{
	UNIT_ASSERT(m_state == State::WAIT_BROADCAST);
	m_broadCmd_cnt++;
	CheckBroadEnd();
}

void BDSvr::ClientRevBroadPartCmd()
{
	UNIT_ASSERT(m_state == State::WAIT_BROADCAST);
	m_broadpartCmd_cnt++;
	CheckBroadEnd();
}

void BDSvr::CheckBroadEnd()
{
	UNIT_ASSERT(m_state == State::WAIT_BROADCAST);
	if (m_broadCmd_cnt == m_mgr.m_bd_client.size() && m_broadpartCmd_cnt == 1)
	{
		UNIT_INFO("finish broadcast test, start discon one client. cid=%llx", m_anyone_sid.cid);
		m_state = State::WAIT_DISCON_ONE;
		UNIT_ASSERT(m_anyone_sid.cid != 0);
		m_mgr.m_svr1.DisconClient(m_anyone_sid);
	}
}

void BDSvr::OnRegResult(uint16 svr_id)
{
}

void BDSvr::OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{

}

void BDSvr::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	UNIT_ASSERT(CMD_VERIFY == cmd);
	string s(msg, msg_len);
	UNIT_ASSERT(s == "verify");
	string rsp_msg = "verify_ok";
	m_mgr.m_svr1.ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
}

void BDSvr::OnClientDisCon(const SessionId &id)
{
	UNIT_INFO("OnClientDisCon cid=%llx", id.cid);
	if (m_state == State::WAIT_DISCON_ONE)
	{
		UNIT_ASSERT(m_anyone_sid.cid == id.cid);
		UNIT_ASSERT(m_anyone_sid.acc_id == id.acc_id);
		UNIT_INFO("discon all");
		m_state = State::WAIT_DISCON_ALL;
		m_mgr.m_svr1.DisconAllClient();
		m_tmp_num = 0;
	}
	else if (m_state == State::WAIT_DISCON_ALL)
	{
		m_tmp_num++;
		if (m_tmp_num == 2)
		{
			m_state = State::END;
			m_mgr.StartCheckRoute();
		}
	}
	else
	{
		UNIT_ASSERT(false);
	}
}

void BDSvr::OnClientConnect(const SessionId &id)
{
	UNIT_ASSERT(m_state == State::WAIT_ALL_VERIFY_OK);
	m_anyone_sid = id;
	bool r = m_client_set.insert(id.cid).second;
	UNIT_ASSERT(r);
	UNIT_INFO("verify SessionId: acc_id=%llx cid=%llx", id.acc_id, id.cid);
	if (m_client_set.size() == m_mgr.m_bd_client.size())
	{//all verify ok
		UNIT_INFO("all verify ok");
		m_state = State::WAIT_BROADCAST;
		string s = "CMD_BROADCAST";
		m_mgr.m_svr1.BroadCastToClient(CMD_BROADCAST, s.c_str(), s.length());
		std::vector<uint64> vec_cid;
		vec_cid.push_back(id.cid);
		m_mgr.m_svr1.BroadCastToClient(vec_cid, CMD_BROADCAST_PART, s.c_str(), s.length());
		m_client_set.clear();
	}
}

void BDSvr::OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id)
{

}

RouteClient::RouteClient(BaseFunTestMgr &mgr)
	:m_mgr(mgr)
{
	m_state = State::WAIT_VERIFY_OK;
	m_tmp1 = 0;
	m_tmp2 = 0;
}

void RouteClient::Start()
{
	UNIT_ASSERT(m_state == State::WAIT_VERIFY_OK);

	m_mgr.m_svr1.m_svr_cb = &m_mgr.m_route_svr1;
	const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
	UNIT_ASSERT(ex_vec.size() > 1);
	UNIT_ASSERT(ConnectInit(ex_vec[0].ip.c_str(), ex_vec[0].port));
}

void RouteClient::SvrRev(uint16 svr_id, uint32 cmd)
{
	if (State::WAIT_NORMAL_MSG_REV == m_state)
	{
		if (1 == svr_id)
		{
			UNIT_ASSERT(cmd == CMD_SVR1_MSG);
			m_tmp1 = 1;
		}
		else if (2 == svr_id)
		{
			UNIT_ASSERT(cmd == CMD_SVR2_MSG);
			m_tmp2 = 1;
		}

		if (m_tmp1 == 1 && 1 == m_tmp2)
		{
			m_state = State::WAIT_CHANGE_ROUTE_MSG_REV;
			UNIT_INFO("change route msg, send svr3 msg to svr2. wait rev");
			m_mgr.m_route_svr1.ChangeRoute();

			auto f = [&]()
			{
				SendStr(CMD_SVR3_MSG, "msg");
			};
			m_tm.StopTimer();
			m_tm.StartTimer(1 * 1000, f);
		}
	}
	else if (State::WAIT_CHANGE_ROUTE_MSG_REV == m_state)
	{
		UNIT_ASSERT(CMD_SVR3_MSG == cmd);
		UNIT_ASSERT(2 == svr_id);
		m_state = State::END;
		m_mgr.End();
	}
	else
	{
		UNIT_ASSERT(false);
	}
}

void RouteClient::OnRecvMsg(uint32 cmd, const std::string &msg)
{
	if (CMD_VERIFY == cmd)
	{
		UNIT_ASSERT(m_state == State::WAIT_VERIFY_OK);
		UNIT_INFO("verify ok");
		m_state = State::WAIT_NORMAL_MSG_REV;
		SendStr(CMD_SVR1_MSG, "msg");
		SendStr(CMD_SVR2_MSG, "msg");
		m_tmp1 = 0;
		m_tmp2 = 0;
	}
	else if (CMD_SVR1_MSG == cmd)
	{
	}
}

void RouteClient::OnConnected()
{
	UNIT_INFO("OnConnected");
	SendStr(CMD_VERIFY, "verify");
}

void RouteClient::OnDisconnected()
{

}

RouteSvr::RouteSvr(BaseFunTestMgr &mgr)
	: m_mgr(mgr)
{
	m_svr_id = 0;
}

void RouteSvr::ChangeRoute()
{
	UNIT_INFO("cid=%llx", m_sid.cid);
	UNIT_ASSERT(m_sid.cid != 0);
	m_mgr.m_svr2.SetMainCmd2Svr(m_sid, BF_SVR3, BF_SVR2);
}

void RouteSvr::OnRegResult(uint16 svr_id)
{

}

void RouteSvr::OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	m_sid = id;
	UNIT_INFO("svr rev msg. m_svr_id =%d cmd=%x id.cid=%llx", m_svr_id, cmd, id.cid);
	m_mgr.m_route_client.SvrRev(m_svr_id, cmd);

}

void RouteSvr::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	UNIT_ASSERT(CMD_VERIFY == cmd);
	string s(msg, msg_len);
	UNIT_ASSERT(s == "verify");
	string rsp_msg = "verify_ok";
	m_mgr.m_svr1.ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
}

void RouteSvr::OnClientDisCon(const SessionId &id)
{

}

void RouteSvr::OnClientConnect(const SessionId &id)
{

}

void RouteSvr::OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id)
{
	UNIT_INFO("OnSetMainCmd2SvrRsp %d %d", main_cmd, svr_id);
}


