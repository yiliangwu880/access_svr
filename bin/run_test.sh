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

function test_combine()
{
	KillProcess "acc_svr"
	sleep 1
	cd svr1
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
	sleep 1
	
	
	KillProcess "./acc_svr"
	echo test_add_acc end
	
	grep "ERROR\|error" ./f_test_combine/OutLog.txt >>  error.txt  #追加
	grep "ERROR\|error" ./svr1/svr_util_log.txt >>  error.txt 
	grep "ERROR\|error" ./svr2/svr_util_log.txt >>  error.txt 
}

#main follow
########################################################################################################
#Init
if [ $# -lt 1 ];then
	echo "run all"
	Init
	test_combine
	for((i=1;i<=30;i++)); do   
	test_combine 
	done  
	#test_add_acc
else
    echo "run submodue" $1
	Init
	$1
fi
cat error.txt

