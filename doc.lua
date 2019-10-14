术语：
	client == 外网客户端，终端，比如手机app
	acc  == access svr
	svr  == 通过acc做转发，和client通讯服务器. 比如业务逻辑服务器，登录服务器, 排行榜服务等。
	ad   == svr 端 acc driver, 驱动，作为库给svr调用。

目的：
{
	大量客户端连接负载均衡。 多个无状态acc
	可复用。 做到无需修改acc进程 代码，直接适用大多数种类服务器。（这个牛掰，不用花两个月时间从0开始开发，反复测试查BUG）
	转发快。后面提到的2个时间快。 接收client数据. 从acc接收io  到 svr接收io的时间。  发送给client数据， 从svr发送io 到 acc 发送io的时间.
}


功能：
{
	acc:
	{
		无状态，支持多台acc
		会话：
		{
			创建 client 和 svr的session.
			svr 主动断开session
			client 断开，session也断开。
			svr 认证client,然后触发创建session. 认证超时失败。
			svr 主动断开 所有session
			保存登录用户ID或者登录的玩家id。 用来其中svr一个设定，然后广播给其他svr.
		}
		svr 连接注册。
		最大连接client数,由svr(认证服务器)请求修改。
		svr请求，设定心跳检查功能，心跳包信息{subcmd, interval, rsp msg}
		svr异常断开，由svr设置是否需要关闭所有svr会话. 例如：由状态的游戏服异常，就关闭所有会话，但无状态的排行榜服异常，就不需要，等排行榜重启，就恢复排行榜所有会话。
		
	}
	
	ad:
	{
		请求注册
		创建 client 和 svr的session.
		svr 主动断开session
		client 断开，session也断开。
		svr 认证client,然后触发创建session.
		svr 主动断开 所有session。
	}

}

协议：
{
	分三层图：
									client             acc                svr
	client和svr层：svr_id,msg			  --------------------------------
	client和acc层：svr_id,msg			  -------------
	acc和svr层:	is_ctrl,union_msg					  	  ----------------

	每层协议说明
	client和svr层：msg 				
	{
		msg 为自定义消息包，比如可以用protobuf。
	}
	client和acc层：svr_id,msg		
	{
		msg == client和svr层：msg 
		svr_id 为 svr 在acc注册的id,用来路由svr用。
	}
	acc和svr层:	is_ctrl,union_msg	
	{
		is_ctrl==1表示控制消息，union_msg为 acc和svr之间的控制消息。union_msg=ctrl_cmd,ctrl_msg
		is_ctrl==0表示转发消息，union_msg为：acc.cid, msg。
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
			uint64 uin;//用户id
		}
		struct SvrConnecter
		{
			uint16 svr_id;
		}
		map<svrid, SvrConnecter> Id2Svr;
		Id2Svr id_2_svr;
	}
	
	ad:
	{
		struct SessionId{
			uint64 cid; //acc的connect id
			uint32 idx; //all_sessions index,也是单个svr内的acc id
		}
		struct Session{
			SessionId id;
			addr;
			uint64 uin;//用户id
		}
		using unordered_map<SessionId, Session> Id2Session;
		vector<Id2Session> all_sessions;
	}

}
