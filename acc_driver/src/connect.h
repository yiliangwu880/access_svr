//驱动具体实现


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/singleton.h"
#include "svr_util/include/easy_code.h"
#include "svr_util/include/typedef.h"
#include "../acc_proto/include/proto.h"
#include "acc_driver.h"

namespace acc {

	static const uint32 RE_CON_INTERVAL_SEC = 10; //x秒尝试重连




	struct Session {
		Session();
		SessionId id;
		std::string remote_ip;
		uint16 remote_port;
	};

	using Id2Session = std::map<SessionId, Session>;

	class AccFacadeMgr;
	class ConMgr;
	//svr连接acc服务器，作为客户端
	class AccClientCon : public lc::ClientCon
	{
	public:
		AccClientCon(AccFacadeMgr &facade, ConMgr &con_mgr, uint32 acc_id);
		bool IsReg() const { return m_is_reg; };

		virtual void OnRecv(const lc::MsgPack &msg) override final;
		virtual void OnConnected() override final;
		virtual void OnDisconnected() override final;

		const Session *FindSession(const SessionId &id);
		//发送 acc::ASMsg
		bool Send(const acc::ASMsg &as_data);
		//发送 acc::ASMsg
		template<class CtrlMsg>
		bool Send(acc::Cmd cmd, const CtrlMsg &send);

	private:
		void HandleRspReg(const ASMsg &msg);
		void HandleCreateSession(const ASMsg &msg);
		void HandleMsgForward(const ASMsg &msg);
		void HandleVerifyReq(const ASMsg &msg);
		void HandleMsgNtfDiscon(const ASMsg &msg);
		void HandleMsgRspSetMainCmd2Svr(const ASMsg &msg);

		void OnTryReconTimeOut();

	private:
		const uint32 m_acc_id;	// ConMgr::m_vec_con 索引
		Id2Session m_id_2_s;
		bool m_is_reg;		//true 表示一台 向acc 注册成功的svr id
		lc::Timer m_recon_tm;	//重连acc定时器
		AccFacadeMgr &m_facade;
		ConMgr &m_con_mgr;
	};


	class ConMgr
	{
	public:
		ConMgr(AccFacadeMgr &facade);
		~ConMgr();
		bool Init(const std::vector<Addr> &vec_addr, uint16 svr_id);
		bool AddAcc(const Addr &addr);
		const std::vector<AccClientCon *> &GetAllCon() const { return m_vec_con; };
		void SetRegSuccess(uint16 svr_id);
		void SetFatal();
		uint16 GetSvrId() const { return m_svr_id; }
		AccClientCon *FindAccClientCon(const SessionId &id) const;
		const Session *FindSession(const SessionId &id) const ;
	private:
		void FreeAllCon();

	private:
		std::vector<AccClientCon *> m_vec_con;
		AccFacadeMgr &m_facade;
		uint16 m_svr_id;
		bool m_is_fatal;      //严重错误状态，断开，不再连接。
	};

	////////////////////define////////////////////
	template<class CtrlMsg>
	bool AccClientCon::Send(acc::Cmd cmd, const CtrlMsg &send)
	{
		std::string tcp_pack;
		acc::ASMsg::Serialize(cmd, send, tcp_pack);
		return SendPack(tcp_pack.c_str(), tcp_pack.length());
	}
}//namespace acc