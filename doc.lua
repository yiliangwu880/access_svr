术语：
	client == 外网客户端，终端，比如手机app
	acc  == access svr
	svr  == 通过acc做转发，和client通讯服务器. 比如业务逻辑服务器，登录服务器, 排行榜服务等。
	ad   == acc driver, 驱动，作为库给svr调用。
	Cmd  ==client和svr通讯的消息号
	main_cmd,sub_cmd == Cmd 由 高16位叫main_cmd,低16位叫sub_cmd组成. main_cmd来实现路由到正确的svr,默认表达svr_id，有时候需要多个svr处理相同cmd,就需要main_cmd动态映射svr_id
	
目的：
{
	可复用。 做到无需修改acc进程 代码，直接适用大多数种类服务器。（这个牛掰，不用花两个月时间从0开始开发，反复测试查BUG）
}

概要功能：
{
	与客户端建立连接。
	消息过滤。 认证svr过的连接，才创建有效会话。
	消息转发，负载均衡。
	业务服务的动态扩展。 提供协议,svr可以请求acc修改路由策略.
	保证玩家在线，切换服务器不需要重新建立连接.
	保持心跳
}

限制： 规定了client 和svr 消息包规则，如下
{
	client和svr层：len, cmd,msg
	{
		len cmd+msg字节数
		cmd 消息号。 uint32, 通过高16位叫main_cmd, 来实现路由到正确的svr。
		msg 为自定义消息包，比如可以用protobuf。
	}
}

具体功能：
{
	acc:
	{
		无状态，支持多台acc
		会话：
		{
			认证成功创建 client 和 svr的session.
			svr 主动断开session
			client 断开，session也断开。
			svr 认证client,然后触发创建session. 等svr认证超时,断开
			svr 主动断开 所有session, 通常svr重启用。
			svr 请求设置 main_cmd映射svr_id
		}
		svr 连接注册。
		最大连接client数,由svr(认证服务器)请求修改。
		svr请求，设定心跳检查功能，心跳包信息{cmd, interval, rsp msg}
		svr请求广播client,分全体和部分
		不保存用户id uin, 需要广播用cid识别。 查找uin对应那个Session，时svr群之间的事情。
	}
	
	ad:
	{
		动态增加acc
		请求注册。
		{
			任意一个连接的acc注册成功算成功。
			部分acc连接失败，可以不用注册。
			任意一个连接的acc注册响应失败，算所有失败，断开。进入无效状态。
			无效状态断开所有出错误日志。 不再重连。 （通常svr群之间没协调好，重复注册。避免问题复杂，就全部断开）
		}
		断线重连。 重连走注册流程。
		创建 client 和 svr的session. 
		{
			认证成功，acc广播svr创建session
		}
		svr 主动断开session
		client 断开，session也断开。
		svr 认证client,
		{
			 svr 需要先请求验证结果，成功或者失败。 再转发client验证通过消息。 避免成功验证，acc接收到client发第二条消息，有可能acc还没收到通知验证成功
			 
		}
		被创建session.
		svr 主动断开 所有session。
		svr请求广播client,分全体和部分
		设置心跳功能
		接收转发client和svr层消息
		请求设置 main_cmd映射svr_id
	}

}

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

具体实现注意项：
{
acc不需要id,避免配置麻烦。它的ip:port就是id
}

实现数据结构：
{
	acc
	{
		struct Session{
			uint64 cid; //acc的connect id
			enum State
			{
				Wait_FIRST_msg, //等client第一条消息。
				Wait_Verify, //已收到第一条消息，转发给svr, 等验证结果。
				Verify, //验证通过
			}
			map<uint16, uint16> MainCmd2SvrId; //有时候需要多个svr处理相同cmd,就需要cmd动态映射svr_id. 比如MMORPG,多个场景进程。
		}
		
		struct SvrConnecter
		{
			uint16 svr_id;
		}
		map<svrid, SvrConnecter> Id2Svr;
		Id2Svr id_2_svr;//管理 连接的svr
	}
	
	ad:
	{
		using unordered_map<SessionId, Session> Id2Session;
		struct SessionId
		{
			uint64 cid; //acc的connect id
			uint32 idx; //也是单个svr内的acc id
		}
		struct Session{
			SessionId id;
			addr;
		}
		struct AccCon{
			..//连接accs
			Id2Session m_id_2_s;
		} 
		vector<AccCon> all_acc_con;
	}

}

测试用例：
{
	演示 账号登录，选择角色，进入游戏， 切换角色
	mmorpg跨场景演示
	棋牌 多个大厅。

}
