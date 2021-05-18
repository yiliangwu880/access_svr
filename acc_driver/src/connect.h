//驱动具体实现


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/singleton.h"
#include "svr_util/include/easy_code.h"
#include "svr_util/include/typedef.h"
#include "../acc_proto/include/proto.h"
#include "acc_driver.h"

namespace acc {

	static const uint32 RE_CON_INTERVAL_SEC = 10; //x秒尝试重连



	using CId2Session = std::map<uint64, Session>;

	class ADFacadeMgr;
	class ConMgr;
	//svr连接acc服务器，作为客户端
	class ADClientCon : public lc::ClientCon
	{
	private:
		const uint32 m_acc_id;	        // ConMgr::m_vec_con 索引
		CId2Session   m_id_2_s;			//acc cid 2 session. 用cid做key就可以了，因为来源都是一个acc。
		bool         m_is_reg;			//true 表示一台 向acc 注册成功的svr id
		lc::Timer    m_recon_tm;	    //重连acc定时器
		ADFacadeMgr &m_facade;
		ConMgr      &m_con_mgr;

	public:
		ADClientCon(ADFacadeMgr &facade, ConMgr &con_mgr, uint32 acc_id);
		~ADClientCon() {};
		bool IsReg() const { return m_is_reg; };

		virtual void OnRecv(const lc::MsgPack &msg) override final;
		virtual void OnConnected() override final;
		virtual void OnDisconnected() override final;

		const Session *FindSession(const SessionId &id);
		//发送 acc::ASMsg
		bool Send(const acc::ASMsg &as_data);
		bool Send(acc::Cmd as_cmd, const std::string &as_msg);
		//发送 acc::ASMsg
		template<class CtrlMsg>
		bool Send(acc::Cmd cmd, const CtrlMsg &send);

	private:
		void HandleRspReg(const ASMsg &msg);
		void HandleCreateSession(const ASMsg &msg);
		void HandleMsgForward(const ASMsg &msg);
		void HandleVerifyReq(const ASMsg &msg);
		void HandleMsgNtfDiscon(const ASMsg &msg);
		void HandleMsgRspSetActiveSvr(const ASMsg &msg);
		void HandleMsgBroadcastUin(const ASMsg &msg);
		void HandleMsgRspCacheMsg(const ASMsg &msg);

		void OnTryReconTimeOut();
	};


	class ConMgr
	{
	private:
		std::vector<ADClientCon *> m_vec_con;
		ADFacadeMgr &m_facade;
		uint16 m_svr_id;
		bool m_is_fatal;      //严重错误状态，断开，不再连接，不再修复，等重启。
		bool m_is_reg; 
		bool m_is_verify_svr;
		std::unique_ptr<MsgAccSeting> m_seting; //如果有信息，ADClientCon连接成功会发送设置心跳信息到acc
	public:
		ConMgr(ADFacadeMgr &facade);
		~ConMgr();
		bool Init(const std::vector<Addr> &vec_addr, uint16 svr_id, bool is_verify_svr);
		bool AddAcc(const Addr &addr, bool is_verify_svr = false);
		const std::vector<ADClientCon *> &GetAllCon() const { return m_vec_con; };
		//致命错误，设置整个svr无效
		//请求任意一个acc注册失效，触发。通常配置错误引起，等重启修复吧。
		void SetFatal();
		uint16 GetSvrId() const { return m_svr_id; }
		bool IsVerify() const { return m_is_verify_svr; }
		ADClientCon *FindADClientCon(const SessionId &id) const;
		const Session *FindSession(const SessionId &id) const;
		void SetRegResult(bool is_success);
		void SetAccSeting(const MsgAccSeting &seting);
		const MsgAccSeting *GetMsgAccSeting() const { return m_seting.get(); }
	private:
		void FreeAllCon();
	};

	////////////////////define////////////////////
	template<class CtrlMsg>
	bool ADClientCon::Send(acc::Cmd cmd, const CtrlMsg &send)
	{
		std::string tcp_pack;
		acc::ASMsg::Serialize(cmd, send, tcp_pack);
		return SendPack(tcp_pack.c_str(), tcp_pack.length());
	}
}//namespace acc