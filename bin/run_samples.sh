#!/bin/sh
#一步运行全部使用例子
#./run_samples.sh 					--测试全部
#./run_samples.sh 子模块函数名     --测试子模块

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
	cp acc_svr ./samples/acc_svr1 -rf
}


function clear()
{
	KillProcess "acc_svr"
}

function SampleEcho()
{
	KillProcess "acc_svr"
	KillProcess "echo_client"
	KillProcess "echo_server"
	sleep 1
	cd ./samples/acc_svr1
	./acc_svr 
	cd -
	
	sleep 1
	
	echo start echo server
	cd samples/echo_server
	StartDaemon ./echo_server
	cd -
	sleep 1
	
	echo start echo cleint
	cd samples/echo_client
	StartDaemon ./echo_client
	cd -
	
	sleep 2;
	KillProcess "acc_svr"
	KillProcess "echo_client"
	KillProcess "echo_server"
	echo echo end
}


#main follow
########################################################################################################
#Init
if [ $# -lt 1 ];then
	echo "run all"
	Init
	SampleEcho
else
    echo "run submodue" $1
	Init
	$1
fi
cat error.txt

