#include "proto.h"

namespace
{
	//简化解包操作。赋值并移动指针
	template<class T>
	void ParseCp(T &dst, const char *&src)
	{
		dst = (decltype(dst))(*src); // 类似 dst = (uint32 &)(*src)
		src = src + sizeof(dst);
	}

	//简化打包操作。赋值并移动指针
	template<class T>
	void SerializeCp(const T &src, char *&dst)
	{
		memcpy(dst, (const char *)&src, sizeof(src));
		dst += sizeof(src);
	}  
}

bool acc::ASMsg::Parse(const char *tcp_pack, uint16 tcp_pack_len)
{
	if (0 == tcp_pack_len || nullptr == tcp_pack)
	{
		return false;
	}
	if (tcp_pack_len< sizeof(cmd))
	{
		return false;
	}
	const char *cur = tcp_pack; //读取指针
	msg_len = tcp_pack_len - sizeof(cmd);
	ParseCp(cmd, cur);
	msg = cur;		
	if (0 == msg_len)
	{
		msg = nullptr;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		return false;
	}
	return  true;
}

bool acc::ASMsg::Serialize(std::string &tcp_pack) const
{
	if (cmd == 0)
	{
		return false;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		return false;
	}
	tcp_pack.clear();
	tcp_pack.append((const char *)&cmd, sizeof(cmd));
	if (nullptr != msg)
	{
		tcp_pack.append(msg, msg_len);
	}
	return true;
}

bool acc::ASMsg::Serialize(Cmd ctrl_cmd, const std::string &as_msg, std::string &tcp_pack)
{
	if (ctrl_cmd == CMD_NONE)
	{
		return false;
	}

	tcp_pack.clear();
	//serialize ASMsg cmg
	tcp_pack.append((const char *)&ctrl_cmd, sizeof(ctrl_cmd));
	tcp_pack.append(as_msg);
	return true;
}

bool acc::ASMsg::Serialize(Cmd ctrl_cmd, const MsgForward &f_msg, std::string &tcp_pack)
{
	if (ctrl_cmd == CMD_NONE)
	{
		return false;
	}

	tcp_pack.clear();
	//serialize ASMsg cmg
	tcp_pack.append((const char *)&ctrl_cmd, sizeof(ctrl_cmd));

	//serialize ASMsg msg
	return f_msg.Serialize(tcp_pack);
}

//bool acc::ASMsg::Serialize(char *tcp_pack, uint16 tcp_pack_len) const
//{
//	if (cmd == 0)
//	{
//		return false;
//	}
//	if (msg_len >= ASMSG_MAX_LEN)
//	{
//		return false;
//	}
//	char *cur = tcp_pack;
//	SerializeCp(cmd, cur);
//	if (nullptr != msg)
//	{
//		memcpy(cur, msg, msg_len);
//	}
//	return true;
//}


bool acc::MsgForward::Parse(const char *tcp_pack, uint16 tcp_pack_len)
{
	if (0 == tcp_pack_len || nullptr == tcp_pack)
	{
		return false;
	}
	if (tcp_pack_len < sizeof(cmd))
	{
		return false;
	}
	const char *cur = tcp_pack; //读取指针

	ParseCp(cid, cur);
	ParseCp(cmd, cur);
	ParseCp(msg_len, cur);
	msg = cur;
	if (0 == msg_len)
	{
		msg = nullptr;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		return false;
	}
	return  true;
}

bool acc::MsgForward::Serialize(std::string &tcp_pack) const
{
	if (cmd == 0)
	{
		return false;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		return false;
	}
	tcp_pack.append((const char *)&cid, sizeof(cid));
	tcp_pack.append((const char *)&cmd, sizeof(cmd));
	tcp_pack.append((const char *)&msg_len, sizeof(msg_len));
	tcp_pack.append(msg, msg_len);
	return true;
}

acc::MsgReqBroadCast::MsgReqBroadCast()
{
	memset(this, 0, sizeof(*this));
}

bool acc::MsgReqBroadCast::Parse(const char *tcp_pack, uint16 tcp_pack_len)
{
	if (0 == tcp_pack_len || nullptr == tcp_pack)
	{
		return false;
	}

	const char *cur = tcp_pack; //读取指针

	ParseCp(cid_len, cur);
	if (0 == cid_len)
	{
		cid_s = nullptr;
	}
	else
	{
		cid_s = (const uint64 *)cur;
		cur += cid_len * sizeof(uint64);
	}

	ParseCp(cmd, cur);
	ParseCp(msg_len, cur);
	msg = cur;
	if (0 == msg_len)
	{
		msg = nullptr;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		return false;
	}
	return  true;
}

bool acc::MsgReqBroadCast::Serialize(std::string &tcp_pack) const
{
	if (cmd == 0)
	{
		return false;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		return false;
	}
	tcp_pack.append((const char *)&cid_len, sizeof(cid_len));
	tcp_pack.append((const char *)cid_s, cid_len * sizeof(uint64));
	tcp_pack.append((const char *)&cmd, sizeof(cmd));
	tcp_pack.append((const char *)&msg_len, sizeof(msg_len));
	tcp_pack.append(msg, msg_len);
	return true;
}


bool acc::MsgReqVerifyRet::Parse(const char *tcp_pack, uint16 tcp_pack_len)
{
	if (0 == tcp_pack_len || nullptr == tcp_pack)
	{
		return false;
	}
	if (tcp_pack_len <= (sizeof(cid) + sizeof(is_success)))
	{
		return false;
	}
	const char *cur = tcp_pack; //读取指针

	ParseCp(cid, cur);
	ParseCp(is_success, cur);
	forward_msg.Parse(cur, tcp_pack_len - sizeof(cid) - sizeof(is_success));
	return  true;
}

bool acc::MsgReqVerifyRet::Serialize(std::string &tcp_pack) const
{
	if (cid == 0)
	{
		return false;
	}

	tcp_pack.append((const char *)&cid, sizeof(cid));
	tcp_pack.append((const char *)&is_success, sizeof(is_success));
	return forward_msg.Serialize(tcp_pack);
}

acc::MsgReqSetHeartbeatInfo::MsgReqSetHeartbeatInfo()
	:cmd(0)
	,rsp_cmd(0)
	,interval_sec(0)
{

}
