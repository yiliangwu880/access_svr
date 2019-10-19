#include "proto.h"

namespace
{
	//简化解包操作。赋值并移动指针
	template<class T>
	void ParseCp(T &dst, const char *&src)
	{
		dst = (decltype(dst))(*src);
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

bool acc::ASMsg::Serialize(uint16 ctrl_cmd, const ForwardMsg &f_msg, std::string &tcp_pack)
{
	if (ctrl_cmd == 0)
	{
		return false;
	}

	tcp_pack.clear();
	//serialize ASMsg cmg
	tcp_pack.append((const char *)&ctrl_cmd, sizeof(ctrl_cmd));

	//serialize ASMsg msg
	return f_msg.Serialize(tcp_pack);
}

bool acc::ASMsg::Serialize(char *tcp_pack, uint16 tcp_pack_len) const
{
	if (cmd == 0)
	{
		return false;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		return false;
	}
	char *cur = tcp_pack;
	SerializeCp(cmd, cur);
	if (nullptr != msg)
	{
		memcpy(cur, msg, msg_len);
	}
	return true;
}


bool acc::ForwardMsg::Parse(const char *tcp_pack, uint16 tcp_pack_len)
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

bool acc::ForwardMsg::Serialize(std::string &tcp_pack) const
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
		cid_s = (decltype(cid_s))cur;
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
