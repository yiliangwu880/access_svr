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

bool acc::ASData::Parse(const char *tcp_pack, uint16 tcp_pack_len)
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
	return  true;

	////////////////////////
	ParseCp(is_ctrl, cur);

	if (is_ctrl > 1)
	{
		return false;
	}
	if (1 == is_ctrl)
	{
		//is_ctrl == 1表示控制消息，union_msg为 acc和svr之间的控制消息。union_msg = ctrl_cmd, ctrl_msg
		auto &ctrl = union_msg.ctrl;
		ParseCp(ctrl.cmd, cur);
		ctrl.msg = cur;
		ctrl.msg_len = tcp_pack_len - sizeof(is_ctrl) - sizeof(ctrl.cmd);
		if (0 == ctrl.msg_len)
		{
			ctrl.msg = nullptr;
		}
	} 
	else
	{
		//is_ctrl == 0表示转发消息，union_msg为：acc.cid, cmd, msg。
		auto &forward_msg = union_msg.forward_msg;
		ParseCp(forward_msg.cid, cur);
		ParseCp(forward_msg.cmd, cur);
		forward_msg.msg = cur;
		forward_msg.msg_len = tcp_pack_len - sizeof(is_ctrl) - sizeof(forward_msg.cid) - sizeof(forward_msg.cmd);
		if (0 == forward_msg.msg_len)
		{
			forward_msg.msg = nullptr;
		}
	}
	return true;
}

bool acc::ASData::Serialize(std::string &tcp_pack) const
{
	tcp_pack.clear();
	tcp_pack.append((const char *)&is_ctrl, sizeof(is_ctrl));
	if (1 == is_ctrl)
	{
		auto &ctrl = union_msg.ctrl;
		if (ctrl.cmd == 0)
		{
			return false;
		}
		tcp_pack.append((const char *)&ctrl.cmd, sizeof(ctrl.cmd)); 
		if (nullptr != ctrl.msg)
		{
			tcp_pack.append(ctrl.msg, ctrl.msg_len);
		}
	}
	else
	{	//is_ctrl == 0表示转发消息，union_msg为：acc.cid, cmd, msg。
		auto &forward_msg = union_msg.forward_msg;
		if (forward_msg.cmd == 0)
		{
			return false;
		}
		tcp_pack.append((const char *)&forward_msg.cid, sizeof(forward_msg.cid));
		tcp_pack.append((const char *)&forward_msg.cmd, sizeof(forward_msg.cmd));
		if (nullptr != forward_msg.msg)
		{
			tcp_pack.append(forward_msg.msg, forward_msg.msg_len);
		}
	}
	return true;
}

bool acc::ASData::Serialize(char *tcp_pack, uint16 tcp_pack_len) const
{
	char *cur = tcp_pack;
	SerializeCp(is_ctrl, cur);
	if (1 == is_ctrl)
	{
		auto &ctrl = union_msg.ctrl;
		if (ctrl.cmd == 0)
		{
			return false;
		}
		SerializeCp(ctrl.cmd, cur);
		if (nullptr != ctrl.msg)
		{
			memcpy(cur, ctrl.msg, ctrl.msg_len);
			cur += ctrl.msg_len;
		}
	}
	else
	{	//is_ctrl == 0表示转发消息，union_msg为：acc.cid, cmd, msg。
		auto &forward_msg = union_msg.forward_msg;
		if (forward_msg.cmd == 0)
		{
			return false;
		}
		SerializeCp(forward_msg.cid, cur);
		SerializeCp(forward_msg.cmd, cur);
		if (nullptr != forward_msg.msg)
		{
			memcpy(cur, forward_msg.msg, forward_msg.msg_len);
			cur += forward_msg.msg_len;
		}
	}
	return true;
}


