/*
协议：
{
	分三层图：
									client             acc                svr
	client和svr层：cmd,msg			  --------------------------------
	client和acc层：cmd,msg			  -------------
	acc和svr层:	as_cmd,as_msg					  			----------------

	每层协议说明
	client和svr层：cmd,msg
	client和acc层：cmd,msg
	{
		cmd 消息号。 uint32, 通过高16位为main_cmd， 来实现路由到正确的svr。 main_cmd通常表达svr_id，有时候需要多个svr处理相同cmd,就需要main_cmd动态映射svr_id
		msg 为自定义消息包，比如可以用protobuf。
	}
	acc和svr层:	as_cmd,as_msg
	{
		as_cmd acc和svr的消息号。
		as_msg 消息体。比如转发消息就为【cid,cmd,msg】。其中cid为 client到acc的连接id
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

	const static uint16 ASMSG_MAX_LEN = 1024 * 4; //4k
	const static uint16 MAX_BROADCAST_CID_NUM = 400; //指定广播 cid最大数量
	//都用linux c++,先不限制字节对齐方式
	//#pragma pack(push)
	//#pragma pack(1)

	//#pragma pack(pop)

	
	//acc和svr之间的消息内容之一
	//当消息号为 CMD_NTF_FORWARD CMD_REQ_FORWARD时，消息体为MsgForward
	struct MsgForward
	{
		uint64 cid;      //其中cid为 client到acc的连接id
		uint32 cmd;      //client和svr层：cmd,msg
		uint16 msg_len;  //msg字节数。网络包不包含这个内容
		const char *msg; //client和svr层：cmd,msg. 注意：你内存指针，别弄野了

		bool Parse(const char *tcp_pack, uint16 tcp_pack_len);
		//@para[out] std::string &tcp_pack
		bool Serialize(std::string &tcp_pack) const;
	};


	//控制消息定义
	enum Cmd : uint16
	{
		CMD_NONE = 0,

		CMD_REQ_REG,		            //MsgReqReg 请求注册 	
		CMD_RSP_REG,				    //MsgRspReg

		CMD_REQ_SET_HEARTBEAT_INFO,		//MsgReqSetHeartbeatInfo 设置心跳功能，可选用

		CMD_REQ_VERIFY_RET,			    //MsgReqVerifyRet 请求验证结果. svr 需要先请求验证结果，再转发client验证通过消息
		CMD_NTF_CREATE_SESSION,			//MsgNtfCreateSession  验证成功的client,从acc通知所有svr,创建会话。

		CMD_NTF_VERIFY_REQ,		        //MsgForward ntf转发client请求认证消息包到svr
		CMD_NTF_FORWARD,		        //MsgForward ntf转发client消息包到svr
		CMD_REQ_FORWARD,			    //MsgForward 请求转发client消息包到client

		CMD_REQ_BROADCAST,              //MsgReqBroadCast 请求全局广播，或者指定cid列表广播.
											
		CMD_REQ_SET_MAIN_CMD_2_SVR,		//MsgReqSetMainCmd2Svr 请求设置 main_cmd映射svr_id.
										// (为了确保client明确知道发送消息到那个svr_id,映射设置需要client发起请求，设置成功通知客户端)
		CMD_RSP_SET_MAIN_CMD_2_SVR,		//MsgRspSetMainCmd2Svr

		CMD_REQ_DISCON,					//MsgReqDiscon 请求acc断开client
		CMD_REQ_DISCON_ALL,				//请求acc断开所有client， 

		CMD_NTF_DISCON,					//MsgNtfDiscon 通知client已断开了

	};

	//acc和svr之间的消息
	struct ASMsg
	{
		Cmd cmd;		//acc,svr之间消息号
		uint16 msg_len;  //msg字节数。
		const char *msg;//自定义内容

		ASMsg()
		{
			memset(this, 0, sizeof(*this));
		}
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
		//bool Serialize(char *tcp_pack, uint16 tcp_pack_len) const;

		//CtrlMsg 必须为固定长度的类型
		//一步把ctrl_cmd ctrl_msg打包成 ASMsg 的tcp_pack
		template<class CtrlMsg>
		static bool Serialize(Cmd ctrl_cmd, const CtrlMsg &ctrl_msg, std::string &tcp_pack)
		{
			ASMsg msg_data;
			msg_data.cmd = ctrl_cmd;
			msg_data.msg_len = sizeof(ctrl_msg);
			msg_data.msg = (char *)&ctrl_msg;

			return msg_data.Serialize(tcp_pack);
		}

		//@tcp_pack [out]  ASMsg对应的序列化字符串
		static bool Serialize(Cmd ctrl_cmd, const MsgForward &forward_msg, std::string &tcp_pack);
	};



	struct MsgNtfDiscon
	{
		uint64 cid;
	};

	//	svr请求，设定心跳检查功能，心跳包信息{cmd, interval, rsp cmd}
	struct  MsgReqSetHeartbeatInfo
	{
		uint32 cmd;		//客户端请求消息号
		uint32 rsp_cmd; //响应给客户端额消息号
		uint64 interval_sec;
	};

	struct MsgReqBroadCast
	{
		uint16 cid_len;		//cid数量
		const uint64 *cid_s;      //cid列表
		uint32 cmd;			//client和svr层：cmd,msg
		uint16 msg_len;     //msg字节数。
		const char *msg;    //client和svr层：cmd,msg. 注意：你内存指针，别弄野了

		bool Parse(const char *tcp_pack, uint16 tcp_pack_len);
		//@para[out] std::string &tcp_pack
		bool Serialize(std::string &tcp_pack) const;
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

	struct MsgNtfCreateSession
	{
		uint64 cid;
	};

	struct MsgReqVerifyRet
	{
		uint64 cid;
		bool is_success;
	};

	struct MsgReqSetMainCmd2Svr
	{
		uint64 cid;
		uint16 main_cmd;
		uint16 svr_id;
	};

	struct MsgRspSetMainCmd2Svr
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
		inline bool Parse(const ASMsg &as_msg, CtrlMsg &msg_ctrl)
		{
			return Parse(as_msg.msg, as_msg.msg_len, msg_ctrl);
		}

		//template<class CtrlMsg>
		//inline void Serialize(const CtrlMsg &msg, std::string &out)
		//{
		//	out.clear();
		//	out.append((char *)&msg, sizeof(msg));
		//}

	}

}