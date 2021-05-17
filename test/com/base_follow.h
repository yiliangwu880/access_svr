/*
	可复用测试client svr.
	acc_driver.h 每个功能的大概跑一遍。
	某些特殊例子不包含。
*/
#pragma once
#include <string>
#include <array>
#include "test_base_fun.h"

class BaseFunTestMgr;
static  const uint16 BF_SVR1 = 1;
static  const uint16 BF_SVR2 = 2;
static  const uint16 BF_SVR3 = 3;

class BaseClient : public lc::ClientCon
{
public:
	void SendStr(uint32 cmd, const std::string &msg);

	virtual void OnRecv(const lc::MsgPack &msg) override;
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) {};
	virtual void OnConnected() {};
	virtual void OnDisconnected() {};
};

class BaseFlowClient : public BaseClient
{
public:
	enum class State
	{
		WAIT_SVR_REG,
		WAIT_VERFIY_RSP,
		WAIT_SVR_RSP,
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;

public:
	BaseFlowClient(BaseFunTestMgr &mgr);
	virtual void OnRecv(const lc::MsgPack &msg) override final;
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) {};
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;

	void SendVerify();
};


class BaseFlowSvr : public ISvrCallBack
{
public:
	enum class State
	{
		WAIT_SVR_REG,
		WAIT_CLIENT_REQ_VERFIY,
		WAIT_CLIENT_MSG,
		WAIT_CLIENT_DISCON,
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;

public:
	BaseFlowSvr(BaseFunTestMgr &mgr);

	//回调注册结果, 失败就是配置错误了，无法修复。重启进程吧。
	//@svr_id = 0表示失败
	virtual void OnRegResult(uint16 svr_id) ;

	//接收client消息包到svr
	virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len) ;

	//接收client消息包.请求认证的包
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) ;

	//client断线通知
	virtual void OnClientDisCon(const SessionId &id) ;

	//client接入，创建会话。 概念类似 新socket连接客户端
	virtual void OnClientConnect(const Session &session) ;

	//@id 请求参数一样
	//@main_cmd 请求参数一样
	//@svr_id 0 表示失败。
	//参考 SetMainCmd2GrpId
	virtual void OnSetMainCmd2GrpIdRsp(const Session &session, uint16 main_cmd, uint16 svr_id) ;
};


class HearBeatClient : public BaseClient
{
public:
	enum class State
	{
		WAIT_VERFIY_RSP, //send verify req, wait rsp
		WAIT_BEAT_RSP,//send heartbeat, wait rsp heartbeat
		WAIT_DISCONNECTED,//stop send heartbeat, wait acc disconnect.
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;

public:
	HearBeatClient(BaseFunTestMgr &mgr);
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;
};

// svr连接对象还是复用 BaseFlowSvr创建的
class HearBeatSvr : public ISvrCallBack
{
public:
	enum class State
	{
		WAIT_VERFIY_REQ, 
		WAIT_CLIENT_BEAT_TIME_OUT, //set beathear info , start client connect, wait client beat
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;

public:
	HearBeatSvr(BaseFunTestMgr &mgr);
	void Start();
	//回调注册结果, 失败就是配置错误了，无法修复。重启进程吧。
	//@svr_id = 0表示失败
	virtual void OnRegResult(uint16 svr_id);

	//接收client消息包到svr
	virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len);

	//接收client消息包.请求认证的包
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

	//client断线通知
	virtual void OnClientDisCon(const SessionId &id);

	//client接入，创建会话。 概念类似 新socket连接客户端
	virtual void OnClientConnect(const Session &session);

	//@id 请求参数一样
	//@main_cmd 请求参数一样
	//@svr_id 0 表示失败。
	//参考 SetMainCmd2GrpId
	virtual void OnSetMainCmd2GrpIdRsp(const Session &session, uint16 main_cmd, uint16 svr_id);
};



//RUN_BROADCAST_DISCON client
class BDClient : public BaseClient
{
public:
	enum class State
	{
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;
	uint32 m_id; //id,0开始，数组索引

public:
	BDClient(BaseFunTestMgr &mgr);
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;
};

//RUN_BROADCAST_DISCON svr
class BDSvr : public ISvrCallBack
{
public:
	enum class State
	{
		WAIT_ALL_CLIENT_CONNECT, //
		WAIT_ALL_VERIFY_OK,
		WAIT_BROADCAST, //broadcast all, broadcast one client,
		WAIT_DISCON_ONE,//discon one client
		WAIT_DISCON_ALL, //, discon all client.
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;
	std::set<uint64> m_client_set; //统计client集合
	uint32 m_broadCmd_cnt;
	uint32 m_broadpartCmd_cnt;
	SessionId m_anyone_sid;
	uint32 m_tmp_num;
public:
	BDSvr(BaseFunTestMgr &mgr);
	void Start();
	void ClientOnConnected(uint32 idx);
	void ClientRevBroadCmd();
	void ClientRevBroadPartCmd();
	void CheckBroadEnd();
	//回调注册结果, 失败就是配置错误了，无法修复。重启进程吧。
	//@svr_id = 0表示失败
	virtual void OnRegResult(uint16 svr_id);

	//接收client消息包到svr
	virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len);

	//接收client消息包.请求认证的包
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

	//client断线通知
	virtual void OnClientDisCon(const SessionId &id);

	//client接入，创建会话。 概念类似 新socket连接客户端
	virtual void OnClientConnect(const Session &session);

	//@id 请求参数一样
	//@main_cmd 请求参数一样
	//@svr_id 0 表示失败。
	//参考 SetMainCmd2GrpId
	virtual void OnSetMainCmd2GrpIdRsp(const Session &session, uint16 main_cmd, uint16 svr_id);
};

//RUN_ROUTE
class RouteClient : public BaseClient
{
public:
	enum class State
	{
		WAIT_VERIFY_OK,
		WAIT_NORMAL_MSG_REV,//send svr1 svr2 msg, wait rev
		WAIT_CHANGE_ROUTE_MSG_REV, //change route msg, send svr3 msg to svr2. wait rev
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;
	uint32 m_tmp1;
	uint32 m_tmp2;
	lc::Timer m_tm;
public:
	RouteClient(BaseFunTestMgr &mgr);
	void Start();
	void SvrRev(uint16 svr_id, uint32 cmd);
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;
};

//RUN_ROUTE
class RouteSvr : public ISvrCallBack
{
public:
	enum class State
	{
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;
	uint16 m_svr_id;
	SessionId m_sid;
public:
	RouteSvr(BaseFunTestMgr &mgr);

	void ChangeRoute();

	//回调注册结果, 失败就是配置错误了，无法修复。重启进程吧。
	//@svr_id = 0表示失败
	virtual void OnRegResult(uint16 svr_id);

	//接收client消息包到svr
	virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len);

	//接收client消息包.请求认证的包
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

	//client断线通知
	virtual void OnClientDisCon(const SessionId &id);

	//client接入，创建会话。 概念类似 新socket连接客户端
	virtual void OnClientConnect(const Session &session);

	//@id 请求参数一样
	//@main_cmd 请求参数一样
	//@svr_id 0 表示失败。
	//参考 SetMainCmd2GrpId
	virtual void OnSetMainCmd2GrpIdRsp(const Session &session, uint16 main_cmd, uint16 svr_id);
};


class BroadcastUinClient : public BaseClient
{
public:
	enum class State
	{
		WAIT_VERIFY_OK,
		WAIT_BROADCAST, //svr1 broadcast uin to svr2 svr3 ,wait rev all
		END,
	};

	BaseFunTestMgr &m_mgr;
	State m_state;
	bool m_svr2_rev;
	bool m_svr3_rev;
	uint64 m_uin;
public:
	BroadcastUinClient(BaseFunTestMgr &mgr);
	void Start();
	void SvrRevUin(uint16 svr_id, uint64 uin);

	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;
};


class BroadcastUinSvr : public ISvrCallBack
{
public:

	BaseFunTestMgr &m_mgr;
	uint16 m_svr_id;
	SessionId m_sid;
public:
	BroadcastUinSvr(BaseFunTestMgr &mgr);
	void BroadcastUin(uint64 uin);
	AllADFacadeMgr &GetFacade();

	virtual void OnRevBroadcastUinToSession(uint64 uin) ;
	//回调注册结果, 失败就是配置错误了，无法修复。重启进程吧。
	//@svr_id = 0表示失败
	virtual void OnRegResult(uint16 svr_id) {};

	//接收client消息包到svr
	virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len) {};

	//接收client消息包.请求认证的包
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

	//client断线通知
	virtual void OnClientDisCon(const SessionId &id) {};

	//client接入，创建会话。 概念类似 新socket连接客户端
	virtual void OnClientConnect(const Session &session) {};

	//@id 请求参数一样
	//@main_cmd 请求参数一样
	//@svr_id 0 表示失败。
	//参考 SetMainCmd2GrpId
	virtual void OnSetMainCmd2GrpIdRsp(const Session &session, uint16 main_cmd, uint16 svr_id) {};
};


//基本功能，流程测试
class BaseFunTestMgr 
{
public:
	enum class State
	{
		RUN_CORRECT_FOLLOW,
		RUN_HEARBEAT,
		RUN_BROADCAST_DISCON, //测试广播，踢人
		RUN_ROUTE,//路由设置
		RUN_BROADCAST_UIN,
		END,
	};

	//step 1  RUN_CORRECT_FOLLOW
	BaseFlowClient m_client;
	BaseFlowSvr m_svr; //代表AllADFacadeMgr::Ins()的状态机，会改变为无效。

	//step 2 心跳测试
	HearBeatClient m_h_client;
	HearBeatSvr m_h_svr;

	//step3 broad cast dicon client
	std::array<BDClient,3> m_bd_client;
	BDSvr m_bd_svr;


	//step4 test route
	RouteSvr m_route_svr1;
	RouteSvr m_route_svr2;
	RouteClient m_route_client;

	//step5 test broadcast uin
	BroadcastUinSvr m_b_svr1;
	BroadcastUinSvr m_b_svr2;
	BroadcastUinSvr m_b_svr3;
	BroadcastUinClient m_b_client;

	State m_state;
	lc::Timer m_tm;
	AllADFacadeMgr m_svr1;
	AllADFacadeMgr m_svr2;
	AllADFacadeMgr m_svr3;
public:
	BaseFunTestMgr();
	bool Init();
	void StartHeartbeatTest();
	void CheckBearHeatEnd();
	void StartCheckBD();//TEST RUN_BROADCAST_DISCON
	void StartCheckRoute();//TEST RUN_ROUTE
	void StartCheckBroadcastUin();
	virtual void End();

};
