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
	cp acc_svr ./svr1 -rf
	cp acc_svr ./svr2 -rf
	cp acc_svr ./svr3 -rf
	cp test_combine ./f_test_combine -rf
	
	rm error.txt

	#remove all old log.
	all_fold_name_list=(
	f_test_combine
	)
    for v in ${all_fold_name_list[@]} ;do
		echo $v
		rm ./${v}/utLog.txt
		rm ./${v}/svr_util_log.txt
    done
}

function test_combine()
{
	KillProcess "acc_svr"
	cd svr1
	./acc_svr 
	cd -
	
	sleep 1
	cd f_test_combine
	./test_combine > OutLog.txt
	cd -
	sleep 1
	
	KillProcess "./acc_svr"
	echo CombineTest end
	
	grep "ERROR\|error" ./f_test_combine/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./svr1/svr_util_log.txt >>  error.txt 
}

function TestGroup()
{
	KillProcess "./acc_svr"
	cd combine_svr
	./acc_svr 
	cd -
	
	sleep 1
	cd f_test_group
	./test_group > OutLog.txt
	cd -
	sleep 1
	
	KillProcess "./acc_svr"
	echo test_group end
	
	grep "ERROR\|error" ./f_test_group/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./f_test_group/svr_util_log.txt >>  error.txt 
	grep "ERROR\|error" ./combine_svr/svr_util_log.txt >>  error.txt 
}



function TestRecon()
{
	echo restart1
	cd ReconSvr
	StartDaemon ./acc_svr 
	cd -
	
	sleep 2
	StartDaemon ./test_recon
	sleep 2
	
	#reconnect 1
	echo cd ReconSvr
	cd ReconSvr
	echo restart2
	KillProcess "./acc_svr"
	sleep 1
	StartDaemon ./acc_svr 
	

	#reconnect 2
	echo restart3
	sleep 3
	KillProcess "./acc_svr"
	sleep 1
	StartDaemon ./acc_svr 
	
	
	sleep 1
	KillProcess acc_svr
	KillProcess test_recon
	echo TestRecon end
	cd -
	grep "ERROR\|error" svr_util_log.txt >>  error.txt 
	grep "ERROR\|error" lc_log.txt >>  error.txt 
}

function TestMoreMfSvr()
{
	 KillProcess "./acc_svr"
	 cd FTestMoreSvr
	 StartDaemon ./test_more_svr
	 cd -
	
	sleep 2
	cd svr1
	StartDaemon ./acc_svr 
	cd -
	cd svr2
	StartDaemon ./acc_svr 
	cd -
	cd svr3
	StartDaemon ./acc_svr 
	cd -
	sleep 2
	
	#del two svr
	KillOneProcess acc_svr
	KillOneProcess acc_svr
	sleep 4
	#start two svr
	cd svr1
	StartDaemon ./acc_svr 
	cd -
	cd svr2
	StartDaemon ./acc_svr 
	cd -
	sleep 2
	
	#del old svr
	KillOneProcess acc_svr
	sleep 4
	KillProcess test_more_svr
	KillProcess "./acc_svr"
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
	Init
	test_combine
else
    echo "run submodue" $1
	Init
	$1
fi
cat error.txt

