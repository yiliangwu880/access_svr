简介：
	很多游戏项目，几乎都是复制别的项目的接入服务器，代码需要根据业务逻辑修修改改。 花两个月时间反复修改测试查BUG。
	针对这情况，设计了可复用接入服务器。 做到无需修改acc进程 代码，直接适用并不局限于游戏服务器。
	为了简化用户使用，需要限制一点业务层实现。具体参考限制项。

详细功能说明参考： doc.lua文件	

编译方法：
	整个文件夹放到linux目录，安装cmake gcc git等。

	git submodule init      --更新子模块
	git submodule update			
	git checkout -f	master		--强制删掉本地分支， track远程master分支
	在目录 external里面，参考说明编译每个文件夹，生成依赖库。
	主目录执行：sh clearBuild.sh 完成编译

vs浏览代码：
	执行.\vs\run.bat,生成sln文件
	
目录结构：
	bin			==执行文件
	test 		==测试
	samples     ==使用例子
	acc_svr		==接入服源码
	acc_proto	==接入服和业务服协议库
	acc_driver	==接入服驱动库，业务服使用的。
	
使用方法：
	1 编译通过
	2 bin目录生成acc_svr，就是接入服程序。
	3 用户自定义的业务服务器，使用驱动库，可以简单地实现连接接入服。 驱动库include文件和库文件目录： access_svr\acc_driver\include\ access_svr\acc_driver\lib\

