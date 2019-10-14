/*
tcp包: len, tcp_pack.
tcp_pack:ctrl_len,ctrl_cmd, ctrl_pack，custom_pack.
--ctrl_len表示ctrl_cmd,ctrl_pack的总字节数。
--纯控制消息，就没有custom_pack了.
--custom_pack 用户自定义消息包，具体协议格式自定义，比如可以用protobuf。
分二层解析

2） user和user层：custom_pack	--user 之间通讯的自定义协议

1） user和mf层： tcp_pack  --user mf通讯协议，
ctrl_cmd, ctrl_pack 解析出 mf::MsgData

分层图：
	user		  mf				user
user和user层	-------------------	user和user层
user和mf层	--- user和mf层	--	user和mf层
*/

#pragma once

#include <string>
#include <string.h>

#ifndef uint32
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef long long int64;
typedef unsigned char uint8;
typedef int int32;
#endif


namespace mf {

	//都用linux c++,先不限制字节对齐方式
	//#pragma pack(push)
	//#pragma pack(1)

	//#pragma pack(pop)

	struct MsgData
	{
		MsgData()
		{
			memset(this, 0, sizeof(MsgData));
		}
		uint16 ctrl_len; //ctrl_len表示ctrl_cmd, ctrl_pack的总字节数。
		uint16 ctrl_cmd;
		uint16 ctrl_pack_len; //ctrl_pack 字节数
		const char *ctrl_pack; //可以为空，表示无消息体
		//用户自定义消息包
		uint16 custom_len;  //custom_pack字节数。
		const char *custom_pack;//可以为空，表示无消息体

	public:
		//Parse tcp pack
		//注意：MsgData 成员指向 tcp_pack的内存，tcp_pack释放后，MsgData的指针会野。
		//@para uint16 tcp_pack_len, 表示tcp_pack有效长度
		//@para MsgData &msg_data, [out]
		bool Parse(const char *tcp_pack, uint16 tcp_pack_len);

		//@para[in] const MsgData &msg_data, 
		//@para[out] std::string &tcp_pack
		bool Serialize(std::string &tcp_pack) const;
		//@para[out] uint16 tcp_pack_len, 表示 tcp_pack有效字节数。
		//@para[out] char *tcp_pack
		//注意：高效，容易越界
		bool Serialize(char *tcp_pack, uint16 tcp_pack_len) const;

		//一步打包成tcp_pack
		template<class CtrlMsg>
		static bool Serialize(uint16 ctrl_cmd, const CtrlMsg &ctrl_msg, const char *custom_pack, uint16 custom_len, std::string &tcp_pack)
		{
			MsgData msg_data;
			msg_data.ctrl_len = sizeof(ctrl_cmd) + sizeof(ctrl_msg);
			msg_data.ctrl_cmd = ctrl_cmd;
			msg_data.ctrl_pack = (char *)&ctrl_msg;
			msg_data.ctrl_pack_len = sizeof(ctrl_msg);
			msg_data.custom_len = custom_len;
			msg_data.custom_pack = custom_pack;

			return msg_data.Serialize(tcp_pack);
		}
	};


	//控制消息定义
	enum Cmd : uint16
	{
		CMD_NONE = 0,
		CMD_NTF_COM,               //通用响应消息， MsgNtfCom

		CMD_REQ_REG,		       //请求注册 	MsgReqReg, mf注册失败，不用反馈给客户端，mfsvr避免复杂度。 客户端自己定时检查注册失败
		CMD_RSP_REG,				//MsgRspReg

		CMD_REQ_CON,			   //请求连接User MsgReqCon,
		CMD_RSP_CON,			   //MsgRspCon

		CMD_REQ_FORWARD,           //请求转发给User	MsgReqForward

		CMD_NTF_DISCON,			   //通知User连接失败 MsgNtfDiscon. CMD_REQ_FORWARD请求失败也会导致这个响应。

		CMD_REQ_BROADCAST,         //请求广播指定组 MsgReqBroadcast, mf原样发送到各个user
	};

	struct MsgNtfCom
	{
		MsgNtfCom()
			:req_cmd(CMD_NONE)
			, tips_len(0)
		{}
		void Init(Cmd cmd, const char *t);
		Cmd req_cmd; //被反馈的请求消息号

		const char *Tips() const { return tips; }
		bool Parse(const void* data, uint16 len);
		void Serialize(std::string &out) const;
	private:
		uint16 tips_len;
		char tips[200]; //提示
	};

	struct MsgReqReg
	{
		uint32 svr_id;
		uint32 group_id;
	};

	struct MsgNone
	{
		char n;
	};

	struct MsgReqCon
	{
		uint32 dst_id; //目标服务器id
	};
	struct MsgRspCon
	{
		bool is_ok;
		uint32 dst_id; //目标服务器id
	};

	struct MsgReqBroadcast
	{
		uint32 src_id;
		uint32 group_id; //0表示广播全部
	};

	struct MsgReqForward
	{
		uint32 src_id;
		uint32 dst_id;
	};
	struct MsgNtfDiscon
	{
		uint32 svr_id;
	};


	//user和mf层. ctrl_pack的编码解码。 就是MsgReqReg MsgReqForward 等
	namespace CtrlMsgProto
	{

		//ctrl msg pack通用解析
		template<class CtrlMsg>
		bool Parse(const void* data, uint16 len, CtrlMsg &msg)
		{
			if (sizeof(msg) != len)
			{
				return false;
			}
			msg = *((const CtrlMsg *)data);
			return true;
		}

		template<class CtrlMsg>
		inline bool Parse(const MsgData &msg_data, CtrlMsg &msg_ctrl)
		{
			return Parse(msg_data.ctrl_pack, msg_data.ctrl_pack_len, msg_ctrl);
		}

		template<class CtrlMsg>
		inline void Serialize(const CtrlMsg &msg, std::string &out)
		{
			out.clear();
			out.append((char *)&msg, sizeof(msg));
		}

		//MsgNtfCom 特例
		template<>
		inline bool Parse(const void* data, uint16 len, MsgNtfCom &msg)
		{
			return msg.Parse(data, len);
		}
		template<>
		inline	void Serialize(const MsgNtfCom &msg, std::string &out)
		{
			msg.Serialize(out);
		}

	}

}