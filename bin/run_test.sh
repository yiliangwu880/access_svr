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
	cp test_2combine ./f_test_2combine -rf
	cp test_add_acc ./f_test_add_acc -rf
	cp test_svr_revert ./f_test_svr_revert -rf
	
	rm error.txt

	#remove all old log.
	all_fold_name_list=(
	f_test_combine
	f_test_2combine
	f_test_add_acc
	svr1
	svr2
	svr3
	)
    for v in ${all_fold_name_list[@]} ;do
		echo $v
		rm ./${v}/OutLog.txt
		rm ./${v}/svr_util_log.txt
    done
}


function clear()
{
	KillProcess "acc_svr"
}

function test_combine()
{
	KillProcess "acc_svr"
	sleep 1
	cd svr1
	./acc_svr 
	cd -
	cd svr2
	./acc_svr 
	cd -
	cd svr3
	./acc_svr 
	cd -
	
	sleep 1
	
	echo start test_combine
	cd f_test_combine
	./test_combine > OutLog.txt
	cd -
	sleep 1
	
	KillProcess "./acc_svr"
	echo CombineTest end
	
	grep "ERROR\|error" ./f_test_combine/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./svr1/svr_util_log.txt >>  error.txt 
	grep "ERROR\|error" ./svr2/svr_util_log.txt >>  error.txt 
	grep "ERROR\|error" ./svr3/svr_util_log.txt >>  error.txt 
}

function test_2combine()
{
	KillProcess "acc_svr"
	sleep 1
	cd svr1
	./acc_svr 
	cd -
	
	sleep 1
	echo start test_2combine
	cd f_test_2combine
	./test_2combine > OutLog.txt
	cd -
	sleep 1
	
	KillProcess "./acc_svr"
	echo 2CombineTest end
	
	grep "ERROR\|error" ./f_test_2combine/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./svr1/svr_util_log.txt >>  error.txt 
}


function test_add_acc()
{
	KillProcess "acc_svr"
	sleep 1
	cd svr1
	./acc_svr 
	cd -
	cd svr2
	./acc_svr 
	cd -
	
	sleep 1
	echo start test_add_acc
	cd f_test_add_acc
	./test_add_acc > OutLog.txt
	cd -

	KillProcess "./acc_svr"
	echo test_add_acc end
	
	grep "ERROR\|error" ./f_test_add_acc/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./svr1/svr_util_log.txt >>  error.txt 
	grep "ERROR\|error" ./svr2/svr_util_log.txt >>  error.txt 
}

function test_svr_revert()
{
	KillProcess "acc_svr"
	sleep 1
	cd svr1
	./acc_svr 
	cd -
	
	sleep 1
	echo start test_svr_revert
	cd f_test_svr_revert
	./test_svr_revert > OutLog.txt
	cd -

	KillProcess "./acc_svr"
	echo test_svr_revert end
	
	grep "ERROR\|error" ./f_test_svr_revert/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./svr1/svr_util_log.txt >>  error.txt 
}
#main follow
########################################################################################################
#Init
if [ $# -lt 1 ];then
	echo "run all"
	Init
	test_2combine
	test_svr_revert
	test_combine
	test_add_acc
else
    echo "run submodue" $1
	Init
	$1
fi
cat error.txt

