#include "com.h"

bool Com::ASData2MsgPack(const acc::ASData &as_data, lc::MsgPack &msg_pack)
{
	std::string tcp_pack;
	COND_F(as_data.Serialize(tcp_pack));
	COND_F(msg_pack.Set(tcp_pack));
	return true;
}
