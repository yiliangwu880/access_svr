#pragma once
#include "log_def.h"
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_include.h"
#include "../acc_proto/include/proto.h"


//连接外网client的sever connect
class ClientSvrCon : public lc::SvrCon
{
	using MainCmd2SvrId = std::set<uint16, uint16> ;
public:
	ClientSvrCon();
	~ClientSvrCon();
	bool SetVerify();

private:
	virtual void OnRecv(const lc::MsgPack &msg) override;
	virtual void OnConnected() override
	{
	}

private:
	void Forward2VerifySvr(const lc::MsgPack &msg);
	void Forward2Svr(const lc::MsgPack &msg) const;

private:
	enum class State
	{
		INIT,        //刚连接进来，没验证,等第一条消息。
		WAIT_VERIFY, //已收到第一条消息，转发给svr, 等验证结果。
		VERIFYED,      //验证已经通过
	};

	State m_state;
	uint64 m_uin;					//用户id
	MainCmd2SvrId m_cmd_2_svrid;	//有时候需要多个svr处理相同cmd,就需要cmd动态映射svr_id. 比如MMORPG,多个场景进程。

};