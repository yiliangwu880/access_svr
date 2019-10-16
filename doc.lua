术语：
	client == 外网客户端，终端，比如手机app
	acc  == access svr
	svr  == 通过acc做转发，和client通讯服务器. 比如业务逻辑服务器，登录服务器, 排行榜服务等。
	ad   == acc driver, 驱动，作为库给svr调用。
	Cmd  ==client和svr通讯的消息号
	main_cmd,sub_cmd == Cmd 由 高16位叫main_cmd,低16位叫sub_cmd组成. main_cmd来实现路由到正确的svr,默认表达svr_id，有时候需要多个svr处理相同cmd,就需要main_cmd动态映射svr_id

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
			{
				因为acc id在不同svr不一样（连接先后索引）。所以必须通过acc广播用户ID,svr才指定用户ID在那个acc
			}
			svr 请求设置 main_cmd映射svr_id
		}
		svr 连接注册。
		最大连接client数,由svr(认证服务器)请求修改。
		svr请求，设定心跳检查功能，心跳包信息{subcmd, interval, rsp msg}
		svr异常断开，由svr设置是否需要关闭所有svr会话. 例如：由状态的游戏服异常，就关闭所有会话，但无状态的排行榜服异常，就不需要，等排行榜重启，就恢复排行榜所有会话。
		svr请求广播client,分全体和部分
	}
	
	ad:
	{
		请求注册
		创建 client 和 svr的session.
		svr 主动断开session
		client 断开，session也断开。
		svr 认证client,然后触发创建session.
		svr 主动断开 所有session。
		svr请求广播client,分全体和部分
	}

}

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
		cmd 消息号。 
		msg 为自定义消息包，比如可以用protobuf。
	}
	
	acc和svr层:	cmd,msg
	{
		cmd acc和svr的消息号。
		msg 自定义消息内容。比如转发消息就为cid, client和svr层：cmd,msg。其中cid为 client到acc的连接id
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
