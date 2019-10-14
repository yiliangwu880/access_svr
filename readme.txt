介绍：
	服务器间通讯用。消息转发。
	msg_forward的客户端，其实也是服务器，作为其他服务。
	特点：
		多个客户端之间通讯。
		客户端之间定义唯一id,到msg_forward注册后就可以互相通讯了。
		客户端 之间通讯，不需要配置彼此之间的地址,只需要配置msg_forward的地址就能实现通讯。
	
编译方法：
	整个文件夹放到linux目录，安装cmake gcc git等。

	git submodule init      --更新子模块
	git submodule update			
	git checkout -f	master		--强制删掉本地分支， track远程master分支
	去目录 External里面，参考说明编译每个文件夹，生成依赖库。
	主目录执行：sh clearBuild.sh 完成编译

vs浏览代码：
	执行.\vs\run.bat,生成sln文件

功能说明参考： doc.lua文件
	
目录结构：
	bin			==执行文件
	test 		==测试



