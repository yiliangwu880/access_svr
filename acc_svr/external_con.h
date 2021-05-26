#pragma once
#include <unordered_map>
#include <queue>
#include "log_def.h"
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_include.h"
#include "svr_util/include/time/su_timer.h"
#include "../acc_proto/include/proto.h"

class ExternalSvrCon;

struct GroupData
{
	uint16 grpId;
	uint16 svrId;
};
class AccSeting : public Singleton<AccSeting>
{
public:
	AccSeting() 
	{
	}
public:
	acc:: MsgAccSeting m_seting;
	std::unordered_map<uint16, uint16> m_cmd2GrpId;
};

extern AccSeting *g_AccSeting;

//连接外网client的sever connect
class ExternalSvrCon : public lc::SvrCon
{
	using CPointChar = const char *;
	using MainCmd2GrpId = std::unordered_map<uint16, GroupData>;
private:
	static const uint64 VERIFY_TIME_OUT_SEC = 10;
	enum class State
	{
		INIT,        //没验证状态，刚连接进来，或者验证失败后，接收消息会转发验证svr。
		WAIT_VERIFY, //已收到第一条消息，转发给svr, 等验证结果。成功->VERIFYED,失败->INIT
		VERIFYED,    //验证已经通过
	};

	State m_state;
	std::vector<uint16> m_grpId2SvrId;//grp id 当前激活的 svrId. 无或者值为0表示激活的svrId== grpID
	lc::Timer m_wfm_tm;			    //等第一条消息超时定时器
	lc::Timer m_verify_tm;		    //认证超时定时器
	lc::Timer m_heartbeat_tm;	    //心跳超时定时器
	lc::Timer m_cls_tm;			    // client limite size. 最大client数量，断开定时器
	uint64 m_uin;                   //玩家标识
	bool m_isCache = false;			//true 表示缓存消息，暂停转发。
	std::vector<std::string> m_cacheMsg;

	//uo 特有
	uint32 Seed = 0;
	bool Seeded = false;
public:
	ExternalSvrCon();
	~ExternalSvrCon();
	void SetVerify(bool is_success);
	bool IsVerify();
	//发送 client和svr层：cmd,msg 到client
	bool SendMsg(uint32 cmd, const char *msg, uint16 msg_len);
	void SetActiveSvrId(uint16 grpId, uint16 svr_id);
	void SetCache(bool isCache);
	uint64 GetUin()const { return m_uin; }
	void SetUin(uint64 uin) { m_uin = uin; }
private:
	//@param pMsg, len  网络缓存字节
	//return 返回已经读取的字节数
	virtual int OnRawRecv(const char *pMsg, int len) override;
	virtual void OnConnected() override;
	virtual void OnRecv(const lc::MsgPack &msg)override;
private:
	//@param pMsg, len  网络缓存字节
	//return 返回包字节数， 0表示包未接收完整。
	int ParsePacket(const char *pMsg, int len);
	void RevPacket(const char *pMsg, int len);
	bool ClientTcpPack2MsgForward(const lc::MsgPack &msg, acc::MsgForward &f_msg) const;
	bool ClientTcpPack2MsgForward(const char *pMsg, int len, acc::MsgForward &f_msg) const;
	void Forward2VerifySvr(const char *pMsg, int len);
	void Forward2Svr(const char *pMsg, int len);
	void OnWaitFirstMsgTimeOut();
	void OnVerfiyTimeOut();
	void OnHeartbeatTimeOut();

	bool HandleSeed(CPointChar &cur, int &len);
	bool CheckEncrypted(int packetID);
};