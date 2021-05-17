简介：
	很多游戏项目，几乎都是复制别的项目的接入服务器，代码需要根据业务逻辑修修改改。 花两个月时间反复修改测试查BUG。
	针对这情况，设计了可复用接入服务器。 做到无需修改acc进程 代码，直接适用并不局限于游戏服务器。
	为了简化用户使用，需要限制一点业务层实现。具体参考限制项。

术语：
	client 	 == 外网客户端，终端，比如手机app
	acc  	 == access svr
	svr  	 == 通过acc做转发，和client通讯服务器. 比如业务逻辑服务器，登录服务器, 排行榜服务等。
	ad   	 == acc driver, 驱动，作为库给svr调用。
	Cmd  	 == client和svr通讯的消息号
	main_cmd == Cmd 的 高16位。 默认表达svr id, 用来给acc实现路由到正确的svr。 有时候需要多个svr处理相同cmd,就需要请求acc设置 main_cmd动态映射svr_id
	sub_cmd  == Cmd 的 低16位。 子消息号，内容无限制， 用户自定义。 

运行时架构：
{
	client client ...
		|	 |																			
		|	 |
	  acc1	acc2  ...													
		|	|
		|   |
	  svr1 svr2(一台svr连接所有acc)							
}

概要功能：
{
	与客户端建立连接。
	消息过滤。 认证svr过的连接，才创建有效会话。
	消息转发，负载均衡。
	业务服务的动态扩展。 svr可以请求acc修改路由策略.
	保证玩家在线，切换服务器不需要重新建立连接.
	保持心跳 （可选用）
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
			认证成功创建 client 和 svr的session。
			svr 主动断开session。
			client 断开，session 销毁。
			svr 认证client,然后触发所有svr创建session. 等svr认证超时,断开。
			svr 请求设置session的uin,并广播给所有svr.
			svr 请求主动断开 所有session。
			svr 请求设置 main_cmd 映射 grpId。	(例如多个zone 属于一个组)
			svr 请求设置 grpId 中 激活的 svr_id。(每个会话独立设置)(例如多个zone 属于相同的grpId)
			
		}
		svr 连接注册。
		最大连接client数,svr可以请求修改。
		svr请求，设定心跳检查功能，心跳包信息{cmd, interval, rsp msg}
		svr请求广播client,分全体和部分
		会话信息保存登录用户uin， svr重启，恢复svr会话信息的.  (参考： 为什么acc需要玩家登录uin呢)
		验证服务器异常，导致没验证响应。当连接失败处理，断开client.
		client连接成功，不发第一条消息。超时断开
	}
	
	ad:
	{
		动态增加acc
		请求注册。
		{
			任意一个连接的acc注册成功算成功。
			部分acc连接失败，可以不用注册。断线重连。 重连走注册流程。
			任意一个连接的acc注册响应失败，算所有失败，断开。进入无效状态。
			无效状态断开所有出错误日志。 不再重连。 （通常svr群之间没协调好，重复注册。避免问题复杂，就全部断开）
		}
		设置心跳功能
		创建 client 和 svr的session. 
		{
			认证成功，acc广播svr创建session
			svr注册成功，所有在acc已认证的client, 创建session
		}
		svr 主动断开session
		client 断开，session也断开。
		svr 认证client.
		svr 主动断开 所有session。
		svr请求广播client,分全体和部分
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
	acc和svr层:	as_cmd,as_msg					  		----------------

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
			uint64 uin; //登录后玩家id
			enum State
			{
				Wait_FIRST_msg, //等client第一条消息。
				Wait_Verify, //已收到第一条消息，转发给svr, 等验证结果。
				Verify, //验证通过
			}
			map<uint16, uint16> MainCmd2GrpId; 
			vector<uint16> grpId2SvrId; //grp id 当前激活的 svrId
		}
		
		struct SvrConnecter
		{
			uint16 svr_id;
		}
		map<svrid, SvrConnecter> Id2Svr;
		Id2Svr id_2_svr;//管理 连接的svr

		client connector
		{
			Session session;
		}

		多个外部client connector;
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
			uint64 uin; //登录后玩家id, svr必不可少的
			addr;
		}
		struct AccCon{
			..//连接accs
			Id2Session m_id_2_s;
		} 
		vector<AccCon> all_acc_con;
	}

}

为什么acc需要玩家登录uin呢： 
{
	对svr来说，uin就是会话的信息之一。
	acc做服务中心，星型结构，简单:
	{
		缺点：侵入业务逻辑，限制用户。
		优点: 用户实现简单。
		流程：
			某个svr会话初始化uin,通过acc中心广播给其他svr. 
			再通过acc中心通知client uin。
			就能保证client第一条请求uin相关消息时,所有svr会话 uin已经设定。
	}
	如果acc不处理uin, 就需要svr 群之间转发uin。加上acc，就构成网状结构。问题复杂。  比如：
	{
		缺点：用户实现太复杂。
		优点: 不限制业务逻辑。

		登录成功后，有不知道先后顺序的网络处理事件：svr 群之间转发uin， client请求uin的第一条协议。 
		设计不好导致有可能 svr收到uin的请求，这时候svr的会话还没设定uin.
	}
	当然uin不是强制要用，特殊情况可以忽略uin. 对于一个账号多个角色的情况适用。
}

测试用例：
{
	演示 账号登录，选择角色，进入游戏， 切换角色
	mmorpg跨场景演示
	棋牌 多个大厅。

}
