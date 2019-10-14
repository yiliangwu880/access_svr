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

bool mf::MsgData::Parse(const char *tcp_pack, uint16 tcp_pack_len)
{
	if (0 == tcp_pack_len || nullptr == tcp_pack)
	{
		return false;
	}
	const char *cur = tcp_pack; //读取指针

	ParseCp(ctrl_len, cur);

	if (tcp_pack_len < ctrl_len)
	{
		return false;
	}

	if (ctrl_len < sizeof(ctrl_cmd))
	{
		return false;
	}

	ParseCp(ctrl_cmd, cur);

	ctrl_pack_len = ctrl_len - sizeof(ctrl_cmd);
	ctrl_pack = cur;

	//point to custom
	custom_len = tcp_pack_len - ctrl_len - sizeof(ctrl_len);
	if (0 == custom_len)
	{
		return true;
	}
	cur = tcp_pack + sizeof(ctrl_len) + ctrl_len;
	custom_pack = cur;
	return true;
}

bool mf::MsgData::Serialize(std::string &tcp_pack) const
{
	if (0 == ctrl_len)
	{
		return false;
	}
	tcp_pack.clear();
	tcp_pack.append((const char *)&ctrl_len, sizeof(ctrl_len));
	tcp_pack.append((const char *)&ctrl_cmd, sizeof(ctrl_cmd));
	if (nullptr != ctrl_pack)
	{
		tcp_pack.append(ctrl_pack, ctrl_pack_len);
	}
	if (nullptr != custom_pack)
	{
		tcp_pack.append(custom_pack, custom_len);
	}
	return true;
}

bool mf::MsgData::Serialize(char *tcp_pack, uint16 tcp_pack_len) const
{
	{//check illegal
		if (0 == ctrl_len || nullptr == tcp_pack || 0 == tcp_pack_len)
		{
			return false;
		}
		uint16 total_size = sizeof(ctrl_len) + ctrl_len + custom_len;
		if (tcp_pack_len < total_size)
		{
			return false;
		}
	}

	char *cur = tcp_pack;
	SerializeCp(ctrl_len, cur);
	SerializeCp(ctrl_cmd, cur);
	if (nullptr != ctrl_pack)
	{
		memcpy(cur, ctrl_pack, ctrl_pack_len);
		cur += ctrl_pack_len;
	}
	if (nullptr != custom_pack)
	{
		memcpy(cur, custom_pack, custom_len);
		cur += custom_len;
	}
	return true;
}


void mf::MsgNtfCom::Init(Cmd cmd, const char *t)
{
	if (nullptr == t)
	{
		return;
	}
	req_cmd = cmd;
	tips_len = ::strlen(t)+1;
	if (tips_len >= sizeof(tips))
	{
		tips_len = sizeof(tips) - 1;
	}
	memcpy(tips, t, tips_len);
	tips[sizeof(tips) - 1] = 0;
}

bool mf::MsgNtfCom::Parse(const void* data, uint16 len)
{
	if (len < sizeof(req_cmd)  + sizeof(tips_len))
	{
		return false;
	}

	const char *cur = (const char *)data;

	ParseCp(req_cmd, cur);
	ParseCp(tips_len, cur);

	if (len < sizeof(req_cmd) + sizeof(tips_len) + tips_len)
	{
		return false;
	}
	if (tips_len>sizeof(tips))
	{
		tips_len = 0;
		return false;
	}
	memcpy(tips, cur, tips_len);
	return true;
}

void mf::MsgNtfCom::Serialize(std::string &out) const
{
	out.clear();
	out.append((const char *)&req_cmd, sizeof(req_cmd));
	out.append((const char *)&tips_len, sizeof(tips_len));
	if (tips_len > sizeof(tips))
	{
		out.clear();
		return;
	}
	out.append((const char *)tips, tips_len);
}
