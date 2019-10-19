#pragma once
#include "log_def.h"
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_include.h"
#include "svr_util/include/time/su_timer.h"
#include "../acc_proto/include/proto.h"
#include <unordered_map>

class ClientSvrCon;



//连接外网client的sever connect
class ClientSvrCon : public lc::SvrCon
{
	using MainCmd2SvrId = std::map<uint16, uint16> ;
public:
	ClientSvrCon();
	~ClientSvrCon();
	void SetVerify(bool is_success);
	bool IsVerify();
	//发送 client和svr层：cmd,msg 到client
	bool SendMsg(uint32 cmd, const char *msg, uint16 msg_len);
	void SetMainCmd2SvrId(uint16 main_cmd, uint16 svr_id);

private:
	virtual void OnRecv(const lc::MsgPack &msg) override;
	virtual void OnConnected() override
	{
	}

private:
	bool ClientTcpPack2ForwardMsg(const lc::MsgPack &msg, acc::ForwardMsg &f_msg) const;
	void Forward2VerifySvr(const lc::MsgPack &msg);
	void Forward2Svr(const lc::MsgPack &msg) const;
	void OnVerfiyTimeOut();
private:
	static const uint64 VERIFY_TIME_OUT_SEC = 10;
	enum class State
	{
		INIT,        //刚连接进来，没验证,等第一条消息。
		WAIT_VERIFY, //已收到第一条消息，转发给svr, 等验证结果。
		VERIFYED,      //验证已经通过
	};

	State m_state;
	MainCmd2SvrId m_cmd_2_svrid;	//有时候需要多个svr处理相同cmd,就需要cmd动态映射svr_id. 比如MMORPG,多个场景进程。
	lc::Timer m_verify_tm;			//认证超时定时器
};