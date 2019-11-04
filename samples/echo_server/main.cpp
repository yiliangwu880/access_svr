#include <string>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include "svr_util/include/log_file.h"
#include "../com/unit_test.h"
#include "acc_driver/include/acc_driver.h"

using namespace std;
using namespace su;
using namespace lc;
using namespace acc;

class EchoServer : public ADFacadeMgr
{
public:
	EchoServer()
	{
	}
	void Start();
	virtual void OnRegResult(uint16 svr_id);
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);
	virtual void OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len);
};

void EchoServer::Start()
{
	UNIT_INFO("EchoServer Start");
	std::vector<Addr> vec_addr;
	Addr addr;
	addr.ip = "127.0.0.1";
	addr.port = 54831;//连接 acc内部地址
	vec_addr.push_back(addr);
	Init(vec_addr, 1, true);
}

void EchoServer::OnRegResult(uint16 svr_id)
{
	//向acc注册成功
	UNIT_INFO("EchoServer reg ok");
}

void EchoServer::OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len)
{
	//接收client请求登录消息,无条件通过，原消息号返回
	UNIT_INFO("rev verfiy. cmd=%d", cmd);
	string s(msg, msg_len);
	string rsp_msg = "verify_ok";
	ReqVerifyRet(id, true, cmd, rsp_msg.c_str(), rsp_msg.length());
}

void EchoServer::OnRevClientMsg(const Session &session, uint32 cmd, const char *msg, uint16 msg_len)
{
	//接收client 请求消息
	UNIT_INFO("echo msg. cmd=%x", cmd);
	SendToClient(session.id, cmd, msg, msg_len);
}

int main(int argc, char* argv[])
{
	EventMgr::Obj().Init();

	EchoServer svr;
	svr.Start();

	EventMgr::Obj().Dispatch();

	return 0;
}

