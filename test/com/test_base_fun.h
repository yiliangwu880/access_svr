/*
	可复用测试client svr
*/
#pragma once
#include <string>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include "svr_util/include/read_cfg.h"
#include "unit_test.h"
#include "../acc_driver/include/acc_driver.h"

using namespace su;
using namespace acc;
using namespace lc;


class ISvrCallBack
{
public:
	//回调注册结果, 失败就是配置错误了，无法修复。重启进程吧。
	//@svr_id = 0表示失败
	virtual void OnRegResult(uint16 svr_id) = 0;

	//接收client消息包到svr
	virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len) = 0;

	//接收client消息包.请求认证的包
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) = 0;

	//client断线通知
	virtual void OnClientDisCon(const SessionId &id) = 0;

	//client接入，创建会话。 概念类似 新socket连接客户端
	virtual void OnClientConnect(const Session &session) = 0;

	//@id 请求参数一样
	//@main_cmd 请求参数一样
	//@svr_id 0 表示失败。
	//参考 SetMainCmd2Svr
	virtual void OnSetMainCmd2SvrRsp(const Session &session, uint16 main_cmd, uint16 svr_id) = 0;
};

class AllADFacadeMgr : public acc::ADFacadeMgr 
{
public:
	AllADFacadeMgr();
	ISvrCallBack *m_svr_cb;//因为 ADFacadeMgr智能有一个对象，需要多种行为，就间接一层.

	//回调注册结果, 失败就是配置错误了，无法修复。重启进程吧。
	//@svr_id = 0表示失败
	virtual void OnRegResult(uint16 svr_id) {
		m_svr_cb->OnRegResult(svr_id);
	}

	//接收client消息包到svr
	virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len) {
		m_svr_cb->OnRevClientMsg(session, cmd, msg, msg_len);
	}

	//接收client消息包.请求认证的包
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) {
		m_svr_cb->OnRevVerifyReq(id, cmd, msg, msg_len);
	}

	//client断线通知
	virtual void OnClientDisCon(const Session &session) {
		m_svr_cb->OnClientDisCon(session.id);
	}

	//client接入，创建会话。 概念类似 新socket连接客户端
	virtual void OnClientConnect(const Session &session) {
		m_svr_cb->OnClientConnect(session);
	}

	//@id 请求参数一样
	//@main_cmd 请求参数一样
	//@svr_id 0 表示失败。
	//参考 SetMainCmd2Svr
	virtual void OnSetMainCmd2SvrRsp(const Session &session, uint16 main_cmd, uint16 svr_id) {
		m_svr_cb->OnSetMainCmd2SvrRsp(session, main_cmd, svr_id);
	}
};



