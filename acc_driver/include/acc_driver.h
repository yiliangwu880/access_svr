/*
//依赖libevent_cpp库
需要下面写才能工作：

main()
{

		调用本库的api


	EventMgr::Ins().Dispatch();
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
#include "../../acc_proto/include/proto.h"

namespace acc {
	struct Addr
	{
		std::string ip;
		uint16 port;
		bool operator<(const Addr &other) const;
	};

	//用来标识接入acc的client，单cid不够，多个acc 的情况，cid有时候会重复分配相同的。
	struct SessionId
	{
		SessionId();
		uint64 cid;		//acc的connect id
		uint32 acc_id; // acc id。 单个svr范围内的有效, 等于ConMgr::m_vec_con 索引. 注意转递给别的svr就不合适用了。可以用addr识别。
		bool operator<(const SessionId &a) const;
	};	
	struct Session {
		Session();
		SessionId id;
		std::string remote_ip;
		uint16 remote_port;
		uint64 uin; //登录后玩家id
	};

	class ConMgr;

	//外观模式，acc driver 接口
	class ADFacadeMgr 
	{
	private:
		ConMgr &m_con_mgr; //具体状态，和对外接口分离

	public:
		ADFacadeMgr();
		~ADFacadeMgr();

		//acc设置，具体内容参考 MsgAccSeting
		//设置acc最大 client数量.
		//当未注册时，会请求注册后，自动发送给acc。
		//当已经注册了，自动马上发送给acc。
		void SetAccSeting(const MsgAccSeting &seting);

		bool Init(const std::vector<Addr> &vec_addr, uint16 svr_id, bool is_verify_svr = false);

		//运行期，新增acc
		bool AddAcc(const Addr &addr); 

		//请求验证结果. 
		bool ReqVerifyRet(const SessionId &id, bool is_success, uint32 cmd, const char *msg, uint16 msg_len);

		//广播uin 一个svr向所有svr的指定会话
		bool BroadcastUinToSession(const SessionId &id, uint64 uin);

		//发送消息包到client
		bool SendToClient(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

		//广播
		void BroadCastToClient(uint32 cmd, const char *msg, uint16 msg_len);

		//广播部分客户端
		//@vec_cid 为cid列表
		void BroadCastToClient(const std::vector<uint64> &vec_cid, uint32 cmd, const char *msg, uint16 msg_len);

		//请求acc踢掉client
		bool DisconClient(const SessionId &id);

		//请求acc踢掉all client
		void DisconAllClient();

		//请求设置 main_cmd映射 grpId
		bool SetMainCmd2GrpId(const SessionId &id, uint16 main_cmd, uint16 grpId);
		//请求设置 svr_grp_id 中 激活的 svr_id
		bool SetActiveSvrId(const SessionId &id, uint16 grpId, uint16 svrId);
		bool SetCache(const SessionId &id, uint16 isCache);

	public:
		//回调注册结果, 失败就是配置错误了，无法修复。重启进程吧。
		//@svr_id = 0表示失败
		virtual void OnRegResult(uint16 svr_id) = 0;

		//接收client请求认证的包. 也就是第一个包
		virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

		//设置会话自定义映射main_cmd to svr_id
		//@id 请求参数一样
		//@main_cmd 请求参数一样
		//@svr_id 0 表示失败。
		//参考 SetMainCmd2GrpId
		virtual void OnSetMainCmd2GrpIdRsp(const Session &session, uint16 main_cmd, uint16 svr_id);


		virtual void OnSetActiveSvr(const Session &session, uint16 grpId, uint16 svr_id);


		virtual void OnMsgRspCacheMsg(const Session &session, bool isCache);

		//接收client消息包到svr
		virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len);

		//client断线通知
		virtual void OnClientDisCon(const Session &session);

		//client认证成功，创建会话。 概念类似 新socket连接客户端
		virtual void OnClientConnect(const Session &session);

		//acc 断线通知
		virtual void OnAccDisCon(const std::string &acc_ip, uint16 acc_port);

		//当设置uin
		virtual void OnRevBroadcastUinToSession(uint64 uin) {};

	};


}