#include <string>
#include "libevent_cpp/include/include_all.h"
#include "log_def.h"
#include "server.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include <signal.h>
#include "svr_con.h"
#include "client_con.h"

using namespace su;
using namespace std;


class MyLcLog : public lc::ILogPrinter, public Singleton<MyLcLog>
{
public:
	virtual void Printf(lc::LogLv lv, const char * file, int line, const char *fun, const char * pattern, va_list vp) override
	{
		su::LogMgr::Obj().Printf((su::LogLv)lv, file, line, fun, pattern, vp);
	}
};


void OnExitProccess()
{
	L_DEBUG("OnExitProccess");
	lc::EventMgr::Obj().StopDispatch();
}

namespace {
	bool InitSvr()
	{
		{//驱动su::timer
			static lc::Timer loop_tm;
			auto fun = std::bind(&SuMgr::OnTimer, &SuMgr::Obj());
			L_COND_F(loop_tm.StartTimer(30, fun, true));
		}

		SvrCtrlMsgDispatch::Obj().Init();
		if (!Server::Obj().Init())
		{
			L_ERROR("server init fail");
			return false;
		}
		return true;
	}
}

int main(int argc, char* argv[])
{
	SuMgr::Obj().Init();
	L_COND_F(CfgMgr::Obj().Init());

	if (CfgMgr::Obj().IsDaemon())
	{
		//当nochdir为0时，daemon将更改进城的根目录为root(“ / ”)。
		//当noclose为0是，daemon将进城的STDIN, STDOUT, STDERR都重定向到 / dev / null。
		L_COND_F(0==daemon(1, 0));
	}

	//start or stop proccess
	SPMgr::Obj().Check(argc, argv, "acc_svr", OnExitProccess);

	lc::EventMgr::Obj().Init(&MyLcLog::Obj());

	if (!InitSvr())
	{
		return 0;
	}

	lc::EventMgr::Obj().Dispatch();
	return 0;
}

