/*
	测试 svr 连接断开，恢复。步骤：
	svr1 ，svr2 连接acc
	client认证，设置uin. 
	svr2断开，重连。
	svr2 会话，uin恢复。

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
	static const uint64 UIN = 11;

	class FollowMgr;
	//connect to acc client
	class Acc2Client : public BaseClient
	{
	public:
		Acc2Client()
		{
			m_follow = nullptr;
		}
		FollowMgr *m_follow;
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
			m_svr_id = 0;
		}
		virtual ~Svr() {};
		FollowMgr *m_follow;
		uint16 m_svr_id;
		virtual void OnRegResult(uint16 svr_id);

		virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

		//当设置uin
		virtual void OnRevBroadcastUinToSession(uint64 uin);

		//client认证成功，创建会话。 概念类似 新socket连接客户端
		virtual void OnClientConnect(const Session &session);
	};

	class FollowMgr 
	{
	public:
		enum class State
		{
			WAIT_REG, //svr1 ，svr2 连接acc
			WAIT_CLIENT_VERIFY,//client认证
			WAIT_SET_UIN_OK, //设置uin
			WAIT_SVR2_RECON, //svr2断开，重连,等连接成功
			WAIT_SVR2_REVERT, //svr2 会话，uin恢复。
			END,
		};

		State m_state;
		Acc2Client m_client;
		Svr m_svr1;
		Svr *m_p_svr2;
		lc::Timer m_tm;
		uint32 m_reg_cnt;
		SessionId m_sid;
	public:
		FollowMgr()
		{
			m_state = State::WAIT_REG;
			m_client.m_follow = this;
			m_svr1.m_follow = this;
			m_svr1.m_svr_id = 1;
			m_reg_cnt = 0;
		}
		void Init();
		void ReconSvr2();
		void InitNewSvr2();
	};


	void Acc2Client::OnRecvMsg(uint32 cmd, const std::string &msg)
	{
		if (cmd == CMD_VERIFY)
		{
			UNIT_INFO("client verify ok");
			UNIT_ASSERT(m_follow->m_state == FollowMgr::State::WAIT_CLIENT_VERIFY);
			m_follow->m_state = FollowMgr::State::WAIT_SET_UIN_OK;
			UNIT_ASSERT(m_follow->m_sid.cid != 0);
			m_follow->m_svr1.BroadcastUinToSession(m_follow->m_sid, UIN);
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
		
	}


	void FollowMgr::Init()
	{
		std::vector<Addr> inner_vec = CfgMgr::Obj().m_inner_vec;
		inner_vec.resize(1);
		m_svr1.Init(inner_vec, 1, true);
		m_p_svr2 = new Svr;
		m_p_svr2->m_follow = this;
		m_p_svr2->m_svr_id = 2;
		m_p_svr2->Init(inner_vec, 2);
	}

	void FollowMgr::ReconSvr2()
	{

		static auto f = [&]()
		{
			UNIT_INFO("WAIT_SVR2_RECON this=%p", this);
			UNIT_ASSERT(m_state == FollowMgr::State::WAIT_SET_UIN_OK);
			m_state = FollowMgr::State::WAIT_SVR2_RECON;
			UNIT_ASSERT(m_p_svr2);
			delete m_p_svr2;
			m_p_svr2 = nullptr;

			m_tm.StopTimer();
			m_tm.StartTimer(1 * 1000, std::bind(&FollowMgr::InitNewSvr2, this));
		};
		m_tm.StopTimer();
		m_tm.StartTimer(1, f);//等下调用，第一安全，第二等acc释放旧的svr2注册信息。
	}


	void FollowMgr::InitNewSvr2()
	{
		UNIT_ASSERT(nullptr == m_p_svr2);
		m_p_svr2 = new Svr;
		m_p_svr2->m_follow = this;
		m_p_svr2->m_svr_id = 2;
		std::vector<Addr> inner_vec = CfgMgr::Obj().m_inner_vec;
		inner_vec.resize(1);
		m_p_svr2->Init(inner_vec, 2);
		UNIT_INFO("re init svr2, m_state =%d this=%p", m_state, this);
	}

	void Svr::OnRegResult(uint16 svr_id)
	{
		if (FollowMgr::State::WAIT_REG == m_follow->m_state)
		{
			UNIT_INFO("first reg ok");
			m_follow->m_reg_cnt++;

			if (m_follow->m_reg_cnt==2)
			{
				m_follow->m_state = FollowMgr::State::WAIT_CLIENT_VERIFY;

				const std::vector<Addr> &ex_vec = CfgMgr::Obj().m_ex_vec;
				UNIT_ASSERT(ex_vec.size() > 1);
				UNIT_INFO("client connect acc1 port=%d", ex_vec[0].port);
				m_follow->m_client.ConnectInit(ex_vec[0].ip.c_str(), ex_vec[0].port);
				return;
			}
			
		}
		else if (FollowMgr::State::WAIT_SVR2_RECON == m_follow->m_state)
		{
			UNIT_INFO("WAIT_SVR2_RECON ok");
			m_follow->m_state = FollowMgr::State::WAIT_SVR2_REVERT;
		}
		else
		{
			UNIT_ASSERT(false);
		}
	}


	void Svr::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
	{
		UNIT_ASSERT(CMD_VERIFY == cmd);
		string s(msg, msg_len);
		string rsp_msg = "verify_ok";
		m_follow->m_sid = id;
		ReqVerifyRet(id, true, CMD_VERIFY, rsp_msg.c_str(), rsp_msg.length());
	}


	void Svr::OnRevBroadcastUinToSession(uint64 uin)
	{
		UNIT_ASSERT(m_follow->m_state == FollowMgr::State::WAIT_SET_UIN_OK);
		UNIT_ASSERT(m_svr_id == 2);
		m_follow->ReconSvr2();
	}


	void Svr::OnClientConnect(const Session &session)
	{
		UNIT_INFO("OnClientConnect m_svr_id=%d m_follow->m_state=%d", m_svr_id, m_follow->m_state);
		if (m_svr_id == 2 && m_follow->m_state == FollowMgr::State::WAIT_SVR2_REVERT)
		{
			UNIT_INFO("revert session uin=%llx", session.uin);
			UNIT_ASSERT(session.uin == UIN);
			UNIT_ASSERT(session.id.cid == m_follow->m_sid.cid);
			UNIT_ASSERT(session.id.cid != 0);
			m_follow->m_state = FollowMgr::State::END;
			EventMgr::Obj().StopDispatch();
		}

	}

}

UNITTEST(test_svr_revert)
{
	UNIT_ASSERT(CfgMgr::Obj().Init());
	EventMgr::Obj().Init();

	FollowMgr follow;
	follow.Init();

	EventMgr::Obj().Dispatch();

	UNIT_INFO("--------------------test_add_acc end--------------------");

}