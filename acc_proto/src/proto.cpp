#include "proto.h"
#include <limits>

namespace
{
	//简化解包操作。赋值并移动指针
	template<class T>
	void ParseCp(T &dst, const char *&src)
	{
		dst = (decltype(dst))(*src); // 类似 dst = (uint32 &)(*src)
		src = src + sizeof(dst);
	}
	template<class T>
	void ParseVector(T &vec, const char *&cur)
	{
		size_t len;
		ParseCp(len, cur);
		vec.reserve(len);
		for (size_t i = 0; i < len; i++)
		{
			typename T::value_type v;
			ParseCp(v, cur);
			vec.push_back(v);
		}
	}

	//简化打包操作。赋值并移动指针
	//不建议用这个， 推荐用 SerialStrCp，
	template<class T>
	void SerializeCp(const T &src, char *&dst)
	{
		memcpy(dst, (const char *)&src, sizeof(src));
		dst += sizeof(src);
	}  	
	template<class T>
		void SerialStrCp(std::string &tcp_pack, const T &v)
	{
		tcp_pack.append((const char *)&v, sizeof(v));
	}

	template<class T>
	void SerialStrCpVector(std::string &tcp_pack, const T &vec)
	{
		SerialStrCp(tcp_pack, vec.size());
		for (auto &v : vec)
		{
			SerialStrCp(tcp_pack, v);
		}
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
	return broadcast_msg.Parse(cur, tcp_pack_len - (cur -tcp_pack));
}

bool acc::MsgReqBroadCast::Serialize(std::string &tcp_pack) const
{
	if (cid_len >= ASMSG_MAX_LEN/ sizeof(uint64))
	{
		return false;
	}
	tcp_pack.append((const char *)&cid_len, sizeof(cid_len));
	tcp_pack.append((const char *)cid_s, cid_len * sizeof(uint64));
	return broadcast_msg.Serialize(tcp_pack);
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
	rsp_msg.Parse(cur, tcp_pack_len - sizeof(cid) - sizeof(is_success));
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
	return rsp_msg.Serialize(tcp_pack);
}

acc::MsgAccSeting::MsgAccSeting()
{
	hbi.req_cmd = 0;
	hbi.rsp_cmd = 0;
	hbi.interval_sec = 0;
	cli.max_num = std::numeric_limits<uint32>::max();
	cli.rsp_cmd = 0;
	cli.rsp_msg.clear();
	no_msg_interval_sec = 0;
}

bool acc::MsgAccSeting::Parse(const char *tcp_pack, uint16 tcp_pack_len)
{
	if (0 == tcp_pack_len || nullptr == tcp_pack)
	{
		return false;
	}
	const char *cur = tcp_pack; //读取指针

	ParseCp(hbi.req_cmd, cur);
	ParseCp(hbi.rsp_cmd, cur);
	ParseCp(hbi.interval_sec, cur);

	ParseCp(cli.max_num, cur);
	ParseCp(cli.rsp_cmd, cur);

	uint16 max_client_msg_len = 0;
	ParseCp(max_client_msg_len, cur);

	if (max_client_msg_len>= ASMSG_MAX_LEN)
	{
		return false;
	}
	cli.rsp_msg.assign(cur, max_client_msg_len);
	cur += max_client_msg_len;
	ParseCp(no_msg_interval_sec, cur);
	ParseCp(defaultGrpId, cur);
	return true;
}

bool acc::MsgAccSeting::Serialize(std::string &tcp_pack) const
{
	tcp_pack.append((const char *)&hbi.req_cmd, sizeof(hbi.req_cmd));
	tcp_pack.append((const char *)&hbi.rsp_cmd, sizeof(hbi.rsp_cmd));
	tcp_pack.append((const char *)&hbi.interval_sec, sizeof(hbi.interval_sec));

	tcp_pack.append((const char *)&cli.max_num, sizeof(cli.max_num));
	tcp_pack.append((const char *)&cli.rsp_cmd, sizeof(cli.rsp_cmd));

	uint16 max_client_msg_len =  cli.rsp_msg.length();
	tcp_pack.append((const char *)&max_client_msg_len, sizeof(max_client_msg_len));

	tcp_pack.append(cli.rsp_msg);

	tcp_pack.append((const char *)&no_msg_interval_sec, sizeof(no_msg_interval_sec));
	tcp_pack.append((const char *)&defaultGrpId, sizeof(defaultGrpId));
	return true;
}

acc::ClientSvrMsg::ClientSvrMsg()
	:cmd(0)
	,msg_len(0)
	,msg(nullptr)
{

}

bool acc::ClientSvrMsg::Parse(const char *tcp_pack, uint16 tcp_pack_len)
{
	if (0 == tcp_pack_len || nullptr == tcp_pack)
	{
		return false;
	}
	const char *cur = tcp_pack; //读取指针
	ParseCp(cmd, cur);
	ParseCp(msg_len, cur);
	msg = cur;
	if (0 == msg_len)
	{
		msg = nullptr;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		cmd = 0;
		msg_len = 0;
		msg = nullptr;
		return false;
	}
	return  true;
}

bool acc::ClientSvrMsg::Serialize(std::string &tcp_pack) const
{
	if (cmd == 0)
	{
		return false;
	}
	if (msg_len >= ASMSG_MAX_LEN)
	{
		return false;
	}
	tcp_pack.append((const char *)&cmd, sizeof(cmd));
	tcp_pack.append((const char *)&msg_len, sizeof(msg_len));
	tcp_pack.append(msg, msg_len);
	return true;
}

bool acc::MsgReqSetCmd2GrpId::Parse(const char *tcp_pack, uint16 tcp_pack_len)
{
	if (0 == tcp_pack_len || nullptr == tcp_pack)
	{
		return false;
	}

	const char *cur = tcp_pack; //读取指针
	ParseCp(grpId, cur);
	ParseVector(vecCmd, cur);
	if (tcp_pack_len != (cur - tcp_pack))
	{
		return false;
	}
	return true;
}

bool acc::MsgReqSetCmd2GrpId::Serialize(std::string &tcp_pack) const
{
	SerialStrCp(tcp_pack, grpId);
	SerialStrCpVector(tcp_pack, vecCmd);
	return true;
}
