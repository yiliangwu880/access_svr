
性能测试

samples
{
	echo server
		演示 账号登录，选择角色，进入游戏， 切换角色
	mmorpg跨场景演示
	棋牌 多个大厅。
}




优化转发，减少内存复制。 （有空整把，不是性能瓶颈）
{
	优化 bool ConCom::SendData(const MsgPack &msg) 函数，想个最少复制的函数。
}