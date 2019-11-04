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

static const uint32 ECHO_CMD = (1 << 16) | 1;

class EchoClient : public lc::ClientCon
{
public:
	EchoClient()
	{
	}
	void Start();
	void SendStr(uint32 cmd, const std::string &msg)
	{
		string pack;
		pack.append((const char *)&cmd, sizeof(cmd));
		pack.append(msg);
		SendPack(pack);
	}
	virtual void OnRecv(const MsgPack &msg) override;
	virtual void OnConnected() override;
	virtual void OnDisconnected() override {};
};

void EchoClient::OnRecv(const MsgPack &msg)
{
	string str;
	uint32 cmd = 0;
	{
		cmd = *((const uint32 *)msg.data);
		const char *msg_str = msg.data + sizeof(uint32);
		uint32 msg_len = msg.len - sizeof(uint32);
		str.append(msg_str, msg_len);
	}

	if (1== cmd)
	{
		UNIT_INFO("verify ok, send main cmd==1, sub cmd == 1 to svr1");
		SendStr(ECHO_CMD, "test msg");
	}
	else if (ECHO_CMD == cmd)
	{
		UNIT_INFO("rev echo msg:%s", str.c_str());
	}
}

void EchoClient::OnConnected()
{
	SendStr(1, "verify");
}



void EchoClient::Start()
{
	ConnectInit("127.0.0.1", 54832); //连接 acc外部地址
}


int main(int argc, char* argv[])
{
	EventMgr::Obj().Init();

	EchoClient c;
	c.Start();

	EventMgr::Obj().Dispatch();

	return 0;
}

