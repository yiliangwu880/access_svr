#include <string>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include "svr_util/include/log_file.h"
#include "unit_test.h"

using namespace std;
using namespace su;
using namespace lc;

int main(int argc, char* argv[])
{
	UnitTestMgr::Obj().Start();
	return 0;
}

