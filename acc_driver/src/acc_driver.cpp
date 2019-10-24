
#include "acc_driver.h"
#include "../acc_proto/include/proto.h"
#include "log_def.h"
#include "connect.h"

using namespace std;
using namespace lc;
using namespace su;
using namespace acc;



acc::AccFacadeMgr::AccFacadeMgr()
	:m_con_mgr(nullptr)
{
	m_con_mgr = new ConMgr(*this);
}

acc::AccFacadeMgr::~AccFacadeMgr()
{
	delete m_con_mgr;
}

bool acc::AccFacadeMgr::Init(const std::vector<Addr> &vec_addr, uint16 svr_id)
{

	return m_con_mgr->Init(vec_addr, svr_id);
}

bool acc::AccFacadeMgr::AddAcc(const Addr &addr)
{
	return m_con_mgr->AddAcc(addr);
}
