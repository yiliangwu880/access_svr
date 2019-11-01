/*
	测试 ADFacadeMgr::AddAcc
	启动2台 acc, 运行期 调用 ADFacadeMgr::AddAcc 连接如2台acc.
	acc2 关闭，
	再测试下断线重连
	acc2 开启
	结束

*/

#include <string>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include "svr_util/include/read_cfg.h"
#include "unit_test.h"
#include "../acc_driver/include/acc_driver.h"
#include "cfg.h"
#include "base_follow.h"

using namespace std;
using namespace su;
using namespace acc;
using namespace lc;


namespace
{

	static const uint32 CMD_VERIFY =  1;
	static const uint32 CMD_TO_SVR = 1 << 16 | 2;

	class FollowMgr;
	//connect to acc2 client
	class Acc2Client : public BaseClient
	{
	public:
		FollowMgr *m_follow;
		virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
		virtual void OnConnected() override final;
		virtual void OnDisconnected() override final;

	};

	class FollowMgr : public acc::ADFacadeMgr
	{
	public:
		enum class State
		{
			WAIT_REG,
			WAIT_ADD_ACC_MSG,
			END,
		};

		State m_state;
		Acc2Client m_client;
		lc::Timer m_tm;
	public:
		FollowMgr();
		void Init();
		virtual void OnRegResult(uint16 svr_id);

		virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);
		virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len);

	};


	void Acc2Client::OnRecvMsg(uint32 cmd, const std::string &msg)
	{
		if (cmd == CMD_VERIFY)
		{
			UNIT_INFO("client send cmd to svr");
			SendStr(CMD_TO_SVR, "");
		}
	}

	void Acc2Client::OnConnected()
	{
		UNIT_INFO("Acc2Client::OnConnected");
		SendStr(CMD_VERIFY, "");
	}


	void Acc2Client::OnDisconnected()
	{
		UNIT_INFO("client discon, try again");
		auto f = [&]()
		{
			TryReconnect();
		};
		m_follow->m_tm.StopTimer();
		m_follow->m_tm.StartTimer(2 * 1000, f);
	}

	FollowMgr::FollowMgr()
	{
		m_state = State::WAIT_REG;
		m_client.m_follow = this;
	}

	void FollowMgr::Init()
	{
		std::vector<Addr> inner_vec = CfgMgr::Obj().m_inner_vec;
		//const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
		inner_vec.resize(1);
		ADFacadeMgr::Init(inner_vec, 1, true);
	}


	void FollowMgr::OnRegResult(uint16 svr_id)
	{
		UNIT_ASSERT(1 == svr_id);
		if (State::WAIT_REG == m_state)
		{
			UNIT_INFO("reg ok, wait add acc reg");
			{
				const std::vector<Addr> &inner_vec = CfgMgr::Obj().m_inner_vec;
				UNIT_ASSERT(inner_vec.size() > 1);
				UNIT_INFO("add acc2 port = ", inner_vec[1].port);
				AddAcc(inner_vec[1]);
			}
			m_state = State::WAIT_ADD_ACC_MSG;
			auto f = [&]()
			{
				const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
				UNIT_ASSERT(ex_vec.size() > 1);
				UNIT_INFO("client connect acc2 port=%d", ex_vec[1].port);
				m_client.ConnectInit(ex_vec[1].ip.c_str(), ex_vec[1].port);
			};
			m_tm.StartTimer(2 * 1000, f);
		}
		else
		{
			UNIT_ASSERT(false);
		}
	}


	void FollowMgr::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
	{
		UNIT_ASSERT(CMD_VERIFY == cmd);
		string s(msg, msg_len);
		string rsp_msg = "verify_ok";
		ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
	}


	void FollowMgr::OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len)
	{
		UNIT_ASSERT(CMD_TO_SVR == cmd);
		UNIT_INFO("rev msg from client to acc2, to svr");
		UNIT_ASSERT(State::WAIT_ADD_ACC_MSG == m_state);
		m_state = State::END;
		EventMgr::Obj().StopDispatch();
	}

}

UNITTEST(test_add_acc)
{
	UNIT_ASSERT(CfgMgr::Obj().Init());
	EventMgr::Obj().Init();

	FollowMgr follow;
	follow.Init();

	EventMgr::Obj().Dispatch();

	UNIT_INFO("--------------------test_add_acc end--------------------");

}