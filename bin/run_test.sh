#!/bin/sh
#一步测试全部，错误信息输出到 error.txt 
#./run_test.sh 					--测试全部
#./run_test.sh 子模块函数名     --测试子模块

user_name=`whoami`

#$1 进程关键字，会用来grep
function KillProcess(){
    echo "KillProcess $1"
	ps -ef|grep $user_name|grep -v "grep"|grep -v $0|grep $1|awk '{print $2}' |xargs kill -10 &>/dev/null
	
}

#关闭一个进程
#$1 进程关键字，会用来grep
function KillOneProcess(){
    echo "KillProcess $1"
	ps -ef|grep $user_name|grep -v "grep"|grep -v $0|grep $1|awk '{print $2}' | head -n 1|xargs kill -10 &>/dev/null
	
}


#$1 start cmd
function StartDaemon()
{
	if [ $# -lt 1 ];then
		echo "StartDaemon miss para 1"
	fi
	echo StartDaemon $1
	nohup $1 &>/dev/null &
}

function Restart()
{
	KillProcess $1
	StartDaemon $1
}

function Init()
{
	#复制执行文件
	cp mf_svr ./svr1 -rf
	cp mf_svr ./svr2 -rf
	cp mf_svr ./svr3 -rf
	cp mf_svr ./ReconSvr -rf
	cp mf_svr ./combine_svr -rf
	mkdir f_test_combine
	cp test_combine ./f_test_combine -rf
	mkdir f_test_group
	cp test_group ./f_test_group -rf
	mkdir FTestMoreSvr
	cp test_more_svr ./FTestMoreSvr -rf
	
	rm error.txt
	rm log.txt
	rm ./svr1/log.txt
	rm ./svr2/log.txt
	rm ./svr3/log.txt
	rm ./combine_svr/svr_util_log.txt
	rm ./f_test_combine/log.txt
	rm ./f_test_combine/svr_util_log.txt
	rm ./f_test_combine/mylog.txt
	rm ./FTestMoreSvr/log.txt
	rm ./FTestMoreSvr/lc_log.txt
}

function TestGroup()
{
	KillProcess "./mf_svr"
	cd combine_svr
	./mf_svr 
	cd -
	
	sleep 1
	cd f_test_group
	./test_group > OutLog.txt
	cd -
	sleep 1
	
	KillProcess "./mf_svr"
	echo test_group end
	
	grep "ERROR\|error" ./f_test_group/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./f_test_group/svr_util_log.txt >>  error.txt 
	grep "ERROR\|error" ./combine_svr/svr_util_log.txt >>  error.txt 
}

function TestCombine()
{
	KillProcess "./mf_svr"
	cd combine_svr
	./mf_svr 
	cd -
	
	sleep 1
	cd f_test_combine
	./test_combine > OutLog.txt
	cd -
	sleep 1
	
	KillProcess "./mf_svr"
	echo CombineTest end
	
	grep "ERROR\|error" ./f_test_combine/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./combine_svr/svr_util_log.txt >>  error.txt 
}

function TestRecon()
{
	echo restart1
	cd ReconSvr
	StartDaemon ./mf_svr 
	cd -
	
	sleep 2
	StartDaemon ./test_recon
	sleep 2
	
	#reconnect 1
	echo cd ReconSvr
	cd ReconSvr
	echo restart2
	KillProcess "./mf_svr"
	sleep 1
	StartDaemon ./mf_svr 
	

	#reconnect 2
	echo restart3
	sleep 3
	KillProcess "./mf_svr"
	sleep 1
	StartDaemon ./mf_svr 
	
	
	sleep 1
	KillProcess mf_svr
	KillProcess test_recon
	echo TestRecon end
	cd -
	grep "ERROR\|error" svr_util_log.txt >>  error.txt 
	grep "ERROR\|error" lc_log.txt >>  error.txt 
}

function TestMoreMfSvr()
{
	 KillProcess "./mf_svr"
	 cd FTestMoreSvr
	 StartDaemon ./test_more_svr
	 cd -
	
	sleep 2
	cd svr1
	StartDaemon ./mf_svr 
	cd -
	cd svr2
	StartDaemon ./mf_svr 
	cd -
	cd svr3
	StartDaemon ./mf_svr 
	cd -
	sleep 2
	
	#del two svr
	KillOneProcess mf_svr
	KillOneProcess mf_svr
	sleep 4
	#start two svr
	cd svr1
	StartDaemon ./mf_svr 
	cd -
	cd svr2
	StartDaemon ./mf_svr 
	cd -
	sleep 2
	
	#del old svr
	KillOneProcess mf_svr
	sleep 4
	KillProcess test_more_svr
	KillProcess "./mf_svr"
	echo end
	
	cd FTestMoreSvr
	grep "ERROR\|error" lc_log.txt >>  ../error.txt 
	grep "ERROR\|error" svr_util_log.txt >>  ../error.txt 
	cd -
}

function M1()
{
	echo "mmm"
}
#main follow
########################################################################################################
#Init
if [ $# -lt 1 ];then
	echo "run all"
else
    echo "run submodue" $1
	$1
fi
exit
TestCombine
TestGroup
TestRecon
TestMoreMfSvr
cat error.txt

