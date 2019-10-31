/*
	综合测试:全功能测试

*/

#include <string>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include "svr_util/include/read_cfg.h"
#include "unit_test.h"
#include "../acc_driver/include/acc_driver.h"
#include "cfg.h"
#include "base_follow.h"

using namespace std;
using namespace su;
using namespace acc;
using namespace lc;


namespace
{

	class MyFollowTest : public BaseFunTestMgr
	{
	public:
		virtual void End()
		{
			UNIT_INFO("--------------------MyFollowTest test end--------------------");
			EventMgr::Obj().StopDispatch();
		}
	};
	MyFollowTest g_base_follow_mgr;

}

UNITTEST(cd)
{
	UNIT_ASSERT(CfgMgr::Obj().Init());
	EventMgr::Obj().Init();

	bool r = g_base_follow_mgr.Init();
	UNIT_ASSERT(r);

	EventMgr::Obj().Dispatch();

	UNIT_ASSERT(g_base_follow_mgr.m_svr.m_state == BaseFlowSvr::State::END);
}