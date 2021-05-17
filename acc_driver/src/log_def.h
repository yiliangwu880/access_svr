/*
	日志宏
	用来做库时，不给用户使用，避免用户也有log_def.h文件，冲突。 用户可以复制这个文件，或者自己定义新的宏来定义新的实现
*/
#pragma once
#include <string>
#include "svr_util/include/log_file.h"

#define L_FATAL(x, ...)  su::LogMgr::Ins().Printf(su::LL_FATAL, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define L_ERROR(x, ...)  su::LogMgr::Ins().Printf(su::LL_ERROR, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define L_WARN(x, ...)   su::LogMgr::Ins().Printf(su::LL_WARN, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define L_INFO(x, ...)   su::LogMgr::Ins().Printf(su::LL_INFO, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define L_DEBUG(x, ...)  su::LogMgr::Ins().Printf(su::LL_DEBUG, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
//#define L_TRACE(x, ...)  su::LogMgr::Ins().Printf(su::LL_TRACE, __FILE__, __LINE__, __FUNCTION__, x, ##__VA_ARGS__);
#define L_TRACE(x, ...)

//简化if语句写法

//条件非法返回false
#define L_COND_F(cond, ...)\
	do{\
	if(!(cond)){\
	su::LogMgr::Ins().PrintfCond(su::LL_ERROR, __FILE__, __LINE__, __FUNCTION__, "condition fail \[" #cond "]. ", ##__VA_ARGS__); \
	return false;\
	}	\
	}while(0);

//条件非法返回
#define L_COND(cond, ...)\
	do{\
	if(!(cond))	\
	{\
	su::LogMgr::Ins().PrintfCond(su::LL_ERROR, __FILE__, __LINE__, __FUNCTION__, "condition fail \[" #cond "]. ", ##__VA_ARGS__); \
		return; \
	}\
	}while(0);

#define L_COND_R(cond, ret, ...)\
	do{\
	if(!(cond)){\
	su::LogMgr::Ins().PrintfCond(su::LL_ERROR, __FILE__, __LINE__, __FUNCTION__, "condition fail \[" #cond "]. ", ##__VA_ARGS__); \
	return ret;\
	}	\
	}while(0);

#define COND_R(cond, ret)\
	do{\
	if(!(cond)){\
	return ret;\
	}	\
	}while(0);

#define COND(cond)\
	do{\
	if(!(cond))	\
	return; \
	}while(0);

#define L_ASSERT(cond)\
	do{\
	if(!(cond)){\
	su::LogMgr::Ins().Printf(su::LL_ERROR, __FILE__, __LINE__, __FUNCTION__, "assert error"); \
	}	\
	}while(0);



