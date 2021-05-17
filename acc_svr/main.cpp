#include <string>
#include "libevent_cpp/include/include_all.h"
#include "log_def.h"
#include "server.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include <signal.h>
#include "inner_con.h"
#include "external_con.h"

using namespace su;
using namespace std;


class MyLcLog : public lc::ILogPrinter, public Singleton<MyLcLog>
{
public:
	virtual void Printf(lc::LogLv lv, const char * file, int line, const char *fun, const char * pattern, va_list vp) override
	{
		su::LogMgr::Ins().Printf((su::LogLv)lv, file, line, fun, pattern, vp);
	}
};

//执行 ./acc_svr stop的时候停进程
void CheckStopProcess()
{
	if (SingleProgress::Ins().IsExit())
	{
		lc::EventMgr::Ins().StopDispatch();
	}
}

namespace {
	bool InitSvr()
	{
		{//驱动su::timer
			static lc::Timer loop_tm;
			auto fun = std::bind(&SuMgr::OnTimer, &SuMgr::Ins());
			L_COND_F(loop_tm.StartTimer(30, fun, true));
		}
		{//stop process
			static lc::Timer loop_tm;
			auto fun = std::bind(&CheckStopProcess);
			L_COND_F(loop_tm.StartTimer(1000*1, fun, true));
		}
		SvrCtrlMsgDispatch::Ins().Init();
		if (!Server::Ins().Init())
		{
			L_ERROR("server init fail");
			return false;
		}
		return true;
	}
}

int main(int argc, char* argv[])
{
	SuMgr::Ins().Init();
	L_COND_F(CfgMgr::Ins().Init());

	if (CfgMgr::Ins().IsDaemon())
	{
		//当nochdir为0时，daemon将更改进城的根目录为root(“ / ”)。
		//当noclose为0是，daemon将进城的STDIN, STDOUT, STDERR都重定向到 / dev / null。
		L_COND_F(0==daemon(1, 0));
	}

	//start or stop proccess
	SingleProgress::Ins().Check(argc, argv, "acc_svr");

	lc::LogMgr::Ins().SetLogPrinter(MyLcLog::Ins());

	if (!InitSvr())
	{
		return 0;
	}

	lc::EventMgr::Ins().Dispatch();
	return 0;
}

