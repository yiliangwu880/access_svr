/*
	综合测试2： 1内容太多了，分离一部分出来。
	acc最大 client数量
	client 连接超时不发消息。


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

	static const uint32 CMD_VERIFY = 1;
	static const uint32 CMD_MAX_CLIENT_RSP_CMD = 22;

	class FollowMgr;
	//connect to acc client
	class NoMsgClient : public BaseClient
	{
	public:
		NoMsgClient()
		{
			m_follow = nullptr;
		}
		FollowMgr *m_follow;
		virtual void OnDisconnected() override final;
	};

	class TestMaxNumClient : public BaseClient
	{
	public:
		TestMaxNumClient()
		{
			m_follow = nullptr;
			idx = 0;
		}
		FollowMgr *m_follow;
		uint32 idx=0;
		virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
		virtual void OnConnected() override final;
		virtual void OnDisconnected() override final;
	};


	class Svr : public ADFacadeMgr
	{
	public:
		Svr()
		{
			m_follow = nullptr;
		}
		virtual ~Svr() {};
		FollowMgr *m_follow;
		virtual void OnRegResult(uint16 svr_id);
		virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);
	};

	class FollowMgr 
	{
	public:
		enum class State
		{
			WAIT_REG,                    //svr reg, wait svr1 连接acc
			WAIT_CLIENT_NO_MSG_TIMEOUT,  //client_no_msg connect, wait client_no_msg timeout
			WAIT_MAX_CLIENT_REV_MAX_MSG, //disconnect client_no_msg, set max client num. wait max client REV MSG.
			WAIT_MAX_CLIENT_DISCON,      //wait max client discon.
			END,
		};

		State m_state;
		NoMsgClient m_nmc;
		vector<TestMaxNumClient *> m_max_num_clients;
		Svr m_svr;
		lc::Timer m_tm;
	public:
		FollowMgr()
		{
			m_state = State::WAIT_REG;
			m_nmc.m_follow = this;
			for (uint32 i=0; i<3; ++i)
			{
				TestMaxNumClient *p = new TestMaxNumClient();
				p->m_follow = this;
				p->idx = i;
				m_max_num_clients.push_back(p);
			}
			m_svr.m_follow = this;
		}
		~FollowMgr()
		{
			for (auto p : m_max_num_clients)
			{
				delete p;
			}
			m_max_num_clients.clear();
		}
		void Start();
		void Start_WAIT_CLIENT_NO_MSG_TIMEOUT();
		void Start_WAIT_MAX_CLIENT_REV_MAX_MSG();
		void Start_WAIT_MAX_CLIENT_DISCON();
		void End();
	};

	void TestMaxNumClient::OnRecvMsg(uint32 cmd, const std::string &msg)
	{
		UNIT_INFO("rev max client num msg. cmd=%d", cmd);
		if (CMD_MAX_CLIENT_RSP_CMD == cmd)
		{
			UNIT_ASSERT(idx == m_follow->m_max_num_clients.size()-1);//确保最后一个
			UNIT_ASSERT(FollowMgr::State::WAIT_MAX_CLIENT_REV_MAX_MSG == m_follow->m_state);
			UNIT_ASSERT(cmd == CMD_MAX_CLIENT_RSP_CMD);
			UNIT_ASSERT(msg == "max");
			m_follow->Start_WAIT_MAX_CLIENT_DISCON();
		}
	}

	void TestMaxNumClient::OnConnected()
	{
		UNIT_INFO("Acc2Client::OnConnected, send verify");
		SendStr(CMD_VERIFY, "");
	}

	void TestMaxNumClient::OnDisconnected()
	{
		if (FollowMgr::State::WAIT_MAX_CLIENT_DISCON == m_follow->m_state)
		{
			UNIT_INFO("max client num, rev discon");
			m_follow->End();
			for (auto p : m_follow->m_max_num_clients)
			{
				p->DisConnect();
			}
		}
	}

	void NoMsgClient::OnDisconnected()
	{
		UNIT_ASSERT(FollowMgr::State::WAIT_CLIENT_NO_MSG_TIMEOUT == m_follow->m_state);

		m_follow->m_tm.StartTimer(1000, std::bind(&FollowMgr::Start_WAIT_MAX_CLIENT_REV_MAX_MSG, m_follow));
	}


	void FollowMgr::Start()
	{
		UNIT_ASSERT(State::WAIT_REG == m_state);


		MsgAccSeting seting;
		seting.no_msg_interval_sec = 1;
		seting.cli.max_num = 2;
		seting.cli.rsp_cmd = CMD_MAX_CLIENT_RSP_CMD;
		seting.cli.rsp_msg = "max";
		m_svr.SetAccSeting(seting);

		std::vector<Addr> inner_vec = CfgMgr::Obj().m_inner_vec;
		inner_vec.resize(1);
		m_svr.Init(inner_vec, 1, true);
	}


	void FollowMgr::Start_WAIT_CLIENT_NO_MSG_TIMEOUT()
	{
		UNIT_ASSERT(FollowMgr::State::WAIT_REG == m_state);
		m_state = FollowMgr::State::WAIT_CLIENT_NO_MSG_TIMEOUT;
		

		const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
		UNIT_ASSERT(ex_vec.size() > 1);
		UNIT_INFO("no msg client connect acc port=%d", ex_vec[0].port);
		m_nmc.ConnectInit(ex_vec[0].ip.c_str(), ex_vec[0].port);
	}




	void FollowMgr::Start_WAIT_MAX_CLIENT_REV_MAX_MSG()
	{
		UNIT_INFO("disconnect client_no_msg, set max client num. wait rev max msg");
		UNIT_ASSERT(FollowMgr::State::WAIT_CLIENT_NO_MSG_TIMEOUT == m_state);
		m_state = State::WAIT_MAX_CLIENT_REV_MAX_MSG;

		const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
		UNIT_ASSERT(ex_vec.size() > 1);
		for (TestMaxNumClient *p : m_max_num_clients)
		{
			p->ConnectInit(ex_vec[0].ip.c_str(), ex_vec[0].port);
		}
	}

	void FollowMgr::Start_WAIT_MAX_CLIENT_DISCON()
	{
		UNIT_ASSERT(FollowMgr::State::WAIT_MAX_CLIENT_REV_MAX_MSG ==m_state);
		m_state = State::WAIT_MAX_CLIENT_DISCON;
	}


	void FollowMgr::End()
	{
		m_state = State::END;
		EventMgr::Obj().StopDispatch();
	}

	void Svr::OnRegResult(uint16 svr_id)
	{
		UNIT_ASSERT(FollowMgr::State::WAIT_REG == m_follow->m_state);
		UNIT_INFO("svr reg ok");

		m_follow->Start_WAIT_CLIENT_NO_MSG_TIMEOUT();
	}


	void Svr::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
	{
		UNIT_INFO("svr1 rev verfiy");
		UNIT_ASSERT(CMD_VERIFY == cmd);
		string s(msg, msg_len);
		string rsp_msg = "verify_ok";
		ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
	}

}

UNITTEST(test_2combine)
{
	UNIT_ASSERT(CfgMgr::Obj().Init());
	EventMgr::Obj().Init();

	FollowMgr follow;
	follow.Start();

	EventMgr::Obj().Dispatch();

	UNIT_INFO("--------------------test_2combine end--------------------");

}