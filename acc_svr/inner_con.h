#pragma once

#include <string>
#include <vector>
#include <map>
#include "log_def.h"
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_include.h"
#include "../acc_proto/include/proto.h"

class InnerSvrCon;

using HandleMsg =  void (*)(InnerSvrCon &con, const acc::ASMsg &msg);
class SvrCtrlMsgDispatch : public Singleton<SvrCtrlMsgDispatch>
{
public:
	void Init();
	void DispatchMsg(InnerSvrCon &con, const acc::ASMsg &msg);

private:
	std::map<acc::Cmd, HandleMsg> m_cmd_2_handle;
};

using Id2Svr = std::map<uint16, InnerSvrCon*>;
//管理已注册的SvrSvrCon
class RegSvrMgr: public Singleton<RegSvrMgr>
{
public:
	RegSvrMgr();
	bool Insert(uint16 svr_id, InnerSvrCon *pSvr);
	bool Remove(uint16 svr_id); //需要保证SvrSvrCon销毁前 调用，避免野指针
	InnerSvrCon *Find(uint16 svr_id);

	const Id2Svr &GetRegSvr() const { return m_id_2_svr; };
private:
	Id2Svr m_id_2_svr;//svr_id 2 SvrSvrCon
};

//管理验证用的SvrSvrCon
class VerifySvrMgr : public Singleton<VerifySvrMgr>
{
public:
	VerifySvrMgr();
	//轮询负载均衡一台验证服务器
	InnerSvrCon *GetBLVerifySvr();
	bool InsertVerify(InnerSvrCon *pSvr);
	bool RemoveVerify(InnerSvrCon *pSvr);//需要保证SvrSvrCon销毁前 调用，避免野指针

private:
	std::set<InnerSvrCon*> m_verify_svr;
	uint32 m_bl_idx; //负载均衡一台验证服务器索引
};

//连接内网svr的sever connect
class InnerSvrCon : public lc::SvrCon
{
public:
	InnerSvrCon();
	~InnerSvrCon();
	bool RegSvrId(uint16 id);
	//return 0 表示未注册
	uint32 GetSvrId() const { return m_svr_id; }
	void SetVerifySvr();
	bool IsVerifySvr() const { return m_is_verify; }
	//发送 acc::ASMsg
	bool Send(const acc::ASMsg &as_data);
	//发送 acc::ASMsg
	template<class CtrlMsg>
	bool Send(acc::Cmd cmd, const CtrlMsg &send);
private:
	virtual void OnRecv(const lc::MsgPack &msg) override;
	virtual void OnConnected() override
	{
	}

private:
	uint16 m_svr_id;		//保持注册成功的id
	bool m_is_verify;	//true 表示 为验证服务器

};

////////////////////define////////////////////
template<class CtrlMsg>
bool InnerSvrCon::Send(acc::Cmd cmd, const CtrlMsg &send)
{
	std::string tcp_pack;
	acc::ASMsg::Serialize(cmd, send, tcp_pack);
	return SendPack(tcp_pack.c_str(), tcp_pack.length());
}