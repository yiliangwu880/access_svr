#pragma once

#include <string>
#include <vector>
#include <map>
#include "log_def.h"
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_include.h"
#include "../acc_proto/include/proto.h"

class SvrSvrCon;

typedef void (*HandleMsg)(SvrSvrCon &con, const acc::AccSvrMsg &msg);
class SvrCtrlMsgDispatch : public Singleton<SvrCtrlMsgDispatch>
{
public:
	void Init();
	void DispatchMsg(SvrSvrCon &con, const acc::AccSvrMsg &msg);

private:
	std::map<acc::Cmd, HandleMsg> m_cmd_2_handle;
};

//管理已注册的SvrSvrCon
class SvrRegMgr: public Singleton<SvrRegMgr>
{
	using Id2Svr =std::map<uint16, SvrSvrCon*>;
public:
	SvrRegMgr();
	//轮询负载均衡一台验证服务器
	SvrSvrCon *GetBLVerifySvr();
	bool Insert(uint16 svr_id, SvrSvrCon *pSvr);
	bool Remove(uint16 svr_id);
	SvrSvrCon *Find(uint16 svr_id);

	bool InsertVerify(SvrSvrCon *pSvr);
	bool RemoveVerify(SvrSvrCon *pSvr);
private:
	Id2Svr m_id_2_svr;//svr_id 2 SvrSvrCon
	std::set<SvrSvrCon*> m_verify_svr;
	uint32 m_bl_idx; //负载均衡一台验证服务器索引
};

//连接内网svr的sever connect
class SvrSvrCon : public lc::SvrCon
{
public:
	SvrSvrCon();
	~SvrSvrCon();
	bool RegSvrId(uint16 id);
	uint32 GetSvrId() { return m_svr_id; }
	void SetVerifySvr();
	bool IsVerifySvr() const { return m_is_verify; }
	bool Send(const acc::ASData &as_data);
	template<class CtrlMsg>
	bool Send(acc::Cmd cmd, const CtrlMsg &send);
private:
	virtual void OnRecv(const lc::MsgPack &msg) override;
	virtual void OnConnected() override
	{
	}

private:
	uint16 m_svr_id;
	bool m_is_verify;	//true 表示 为验证服务器

};

////////////////////define////////////////////
template<class CtrlMsg>
bool SvrSvrCon::Send(acc::Cmd cmd, const CtrlMsg &send)
{
	std::string tcp_pack;
	acc::ASData::Serialize(cmd, send, tcp_pack);
	lc::MsgPack msg_pack;
	COND_F(msg_pack.Serialize(tcp_pack));
	return SendData(msg_pack);
}