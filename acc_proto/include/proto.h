/*
协议：
{
	分三层图：
									client             acc                svr
	client和svr层：cmd,msg			  --------------------------------
	client和acc层：cmd,msg			  -------------
	acc和svr层:	is_ctrl,union_msg					  	  ----------------

	每层协议说明
	client和svr层：cmd,msg
	client和acc层：cmd,msg
	{
		cmd 消息号。 uint32, 通过高16位来实现路由到正确的svr,通常表达svr_id，有时候需要多个svr处理相同cmd,就需要cmd动态映射svr_id
		msg 为自定义消息包，比如可以用protobuf。
	}
	acc和svr层:	cmd,msg
	{
		cmd acc和svr的消息号。
		msg 自定义消息内容。比如转发消息就为cid, client和svr层：cmd,msg。其中cid为 client到acc的连接id
	}
}
*/

#pragma once

#include <string>
#include <string.h>

#ifndef uint32
using uint16 =unsigned short ;
using uint32 = unsigned int ;
using uint64 = unsigned long long ;
using int64 = long long ;
using uint8 = unsigned char ;
using int32 = int ;
#endif


namespace acc {

	//都用linux c++,先不限制字节对齐方式
	//#pragma pack(push)
	//#pragma pack(1)

	//#pragma pack(pop)

	//acc和svr之间的控制消息
	struct AccSvrMsg
	{
		uint16 cmd;
		uint16 msg_len;  //msg字节数。
		const char *msg;
	};
	struct ForwardMsg
	{
		uint64 cid; //其中cid为 client到acc的连接id
		uint32 cmd;
		uint16 msg_len;  //msg字节数。
		const char *msg;
	};
	//acc和svr层消息解析
	struct ASData
	{
		ASData()
		{
			memset(this, 0, sizeof(ASData));
		}
		///////////////////////////////////
		uint16 cmd;		//acc,svr之间消息号
		uint16 msg_len;  //msg字节数。
		const char *msg;//自定义内容
		//////////////////////////////
		char is_ctrl;
		union 
		{
			//is_ctrl == 1表示控制消息，union_msg为 acc和svr之间的控制消息。union_msg = ctrl_cmd, ctrl_msg
			AccSvrMsg ctrl;
			//is_ctrl == 0表示转发消息，union_msg为：acc.cid, cmd, msg。
			ForwardMsg forward_msg;
		}union_msg;
	public:
		//Parse tcp pack
		//注意：ASData 成员指向 tcp_pack的内存，tcp_pack释放后，ASData的指针会野。
		//@para uint16 tcp_pack_len, 表示tcp_pack有效长度
		bool Parse(const char *tcp_pack, uint16 tcp_pack_len);
 
		//@para[out] std::string &tcp_pack
		bool Serialize(std::string &tcp_pack) const;
		//@para[in] uint16 tcp_pack_len, 表示 tcp_pack有效字节数。
		//@para[out] char *tcp_pack
		//注意：高效，容易越界
		bool Serialize(char *tcp_pack, uint16 tcp_pack_len) const;

		//一步把 CtrlMsg打包成tcp_pack
		template<class CtrlMsg>
		static bool Serialize(uint16 ctrl_cmd, const CtrlMsg &ctrl_msg, std::string &tcp_pack)
		{
			ASData msg_data;
			msg_data.is_ctrl = 1;
			auto &ctrl = msg_data.union_msg.ctrl;
			ctrl.cmd = ctrl_cmd;
			ctrl.msg = (char *)&ctrl_msg;
			ctrl.msg_len = sizeof(ctrl_msg);

			return msg_data.Serialize(tcp_pack);
		}
	};


	//控制消息定义
	enum Cmd : uint16
	{
		CMD_NONE = 0,

		CMD_REQ_REG,		            //MsgReqReg 请求注册 	
		CMD_RSP_REG,				    //MsgRspReg

		CMD_REQ_VERIFY_RET,			    //MsgReqVerifyRet 请求验证结果. svr 需要先请求验证结果，再转发client验证通过消息

		CMD_NTF_FORWARD,		        //MsgNtfFoward ntf转发client消息包到svr
		CMD_REQ_FORWARD,			    //MsgReqFoward 请求转发client消息包到client

		CMD_REQ_BROADCAST,              //MsgReqBroadCast 请求全局广播，或者指定cid列表广播.

		CMD_REQ_SET_MAIN_CMD,		    //MsgReqSetMainCmd 请求设置 main_cmd映射svr_id

		CMD_REQ_DISCON,					//MsgReqDiscon 请求acc断开client

	};

	struct MsgReqReg
	{
		MsgReqReg()
			:svr_id(0)
			, is_verify(false)
		{}
		uint32 svr_id;
		bool is_verify; //true 表示 为验证服务器
	};
	struct MsgRspReg
	{
		uint32 svr_id; //失败返回 0 .
	};

	struct MsgReqVerifyRet
	{
		uint64 cid;
		bool is_success;
	};

	struct MsgReqSetMainCmd
	{
		uint64 cid;
		uint16 main_cmd;
		uint16 svr_id;
	};

	struct MsgReqDiscon
	{
		uint64 cid;
	};

	//acc和svr层. ctrl_msg的编码解码。 
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
		inline bool Parse(const AccSvrMsg &ctrl, CtrlMsg &msg_ctrl)
		{
			return Parse(ctrl.msg, ctrl.msg_len, msg_ctrl);
		}

		template<class CtrlMsg>
		inline void Serialize(const CtrlMsg &msg, std::string &out)
		{
			out.clear();
			out.append((char *)&msg, sizeof(msg));
		}

	}

}