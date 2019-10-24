/*
//依赖libevent_cpp库
需要下面写才能工作：

main()
{
	EventMgr::Obj().Init();

		调用本库的api


	EventMgr::Obj().Dispatch();
}

*/

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

namespace acc {
	struct Addr
	{
		std::string ip;
		uint16 port;
		bool operator<(const Addr &other) const;
	};

	struct SessionId
	{
		SessionId();
		uint64 cid;		//acc的connect id
		uint32 acc_id; // ConMgr::m_vec_con 索引
		bool operator<(const SessionId &a) const;
	};
	class ConMgr;
	//外观模式，给用户使用唯一接口
	class AccFacadeMgr 
	{
	public:
		AccFacadeMgr();
		~AccFacadeMgr();

		bool Init(const std::vector<Addr> &vec_addr, uint16 svr_id);

		//运行期，新增acc
		bool AddAcc(const Addr &addr); 


		//设置心跳
		//@cmd 客户端请求消息号
		//@rsp_cmd 响应给客户端额消息号
		//@interval_sec 心跳超时
		void SetHeartbeatInfo(uint32 cmd, uint32 rsp_cmd, uint64 interval_sec);

		//请求验证结果. svr 需要先请求验证结果，再转发client验证通过消息
		void ReqVerifyRet(const SessionId &id, bool is_success);

		//发送消息包到client
		void SendToClient(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

		//广播
		void BroadCastToClient(uint32 cmd, const char *msg, uint16 msg_len);

		//广播部分客户端
		void BroadCastToClient(const std::vector<uint64> &vec_cid, uint32 cmd, const char *msg, uint16 msg_len);

		//请求acc踢掉client
		void DisconClient(const SessionId &id);

		//请求acc踢掉all client
		void DisconAllClient();

		//请求设置 main_cmd映射svr_id
		void SetMainCmd2Svr(const SessionId &id, uint16 main_cmd, uint16 svr_id);

	public:
		//回调注册结果
		//@svr_id =0表示失败
		virtual void OnRegResult(uint16 svr_id) = 0;

		//接收client消息包到svr
		virtual void OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) = 0;

		//接收client消息包.请求认证的包
		virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) = 0;

		//client断线通知
		virtual void OnClientDisCon(const SessionId &id) = 0;

		virtual void OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id) = 0;

	private:
		ConMgr &m_con_mgr;
	};


}