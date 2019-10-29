#pragma once
#include <string>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include "svr_util/include/read_cfg.h"
#include "unit_test.h"
#include "../acc_driver/include/acc_driver.h"

using namespace su;
using namespace acc;
using namespace lc;

class CfgMgr : public Singleton<CfgMgr>
{
public:
	bool Init()
	{
		m_inner_vec.clear();
		UNIT_INFO("init cfg");
		su::Config cfg;
		UNIT_ASSERT(cfg.init("./cfg.txt"));

		std::string ip = "127.0.0.1";
		m_inner_vec.push_back({ ip.c_str(), (uint16)cfg.GetInt("inner_port1") });
		m_inner_vec.push_back({ ip.c_str(), (uint16)cfg.GetInt("inner_port2") });
		m_inner_vec.push_back({ ip.c_str(), (uint16)cfg.GetInt("inner_port3") });
		m_ex_vec.push_back({ ip.c_str(), (uint16)cfg.GetInt("ex_port1") });
		m_ex_vec.push_back({ ip.c_str(), (uint16)cfg.GetInt("ex_port2") });
		m_ex_vec.push_back({ ip.c_str(), (uint16)cfg.GetInt("ex_port3") });

		return true;
	}
	const std::vector <Addr> &GetVecAddr() const { return m_inner_vec; };

public:
	std::vector<Addr> m_inner_vec; //acc inner addr list
	std::vector<Addr> m_ex_vec; //acc export addr list
};
