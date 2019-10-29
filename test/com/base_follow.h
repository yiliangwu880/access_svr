/*
	�ɸ��ò���client svr.
	ÿ�����ܵ�������������һ�顣

	//���������
	//�ɸ����ۺϹ��ܣ�ʵ�� client svr�� ����acc ���£�
	//svr ����ע��
	//client req verify
	//svr req verify ok
	//create session
	//client send msg
	//svr rev msg
	//client disconnect
	//svr del session

	����
*/
#pragma once
#include <string>
#include "test_base_fun.h"

class BaseFunFollowMgr;
static  const uint16 BF_SVR1 = 1;

class BaseClient : public lc::ClientCon
{
public:
	void SendStr(uint32 cmd, const std::string &msg);

	virtual void OnRecv(const lc::MsgPack &msg) override;
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) = 0;
};

class BaseFlowClient : public BaseClient
{
public:
	enum class State
	{
		WAIT_SVR_REG,
		WAIT_VERFIY_RSP,
		WAIT_SVR_RSP,
		END,
	};

	BaseFunFollowMgr &m_mgr;
	State m_state;

public:
	BaseFlowClient(BaseFunFollowMgr &mgr);
	virtual void OnRecv(const lc::MsgPack &msg) override final;
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) {};
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;

	void SendVerify();
};


class BaseFlowSvr : public ISvrCallBack
{
public:
	enum class State
	{
		WAIT_SVR_REG,
		WAIT_CLIENT_REQ_VERFIY,
		WAIT_CLIENT_MSG,
		WAIT_CLIENT_DISCON,
		END,
	};

	BaseFunFollowMgr &m_mgr;
	State m_state;

public:
	BaseFlowSvr(BaseFunFollowMgr &mgr);

	//�ص�ע����, ʧ�ܾ������ô����ˣ��޷��޸����������̰ɡ�
	//@svr_id = 0��ʾʧ��
	virtual void OnRegResult(uint16 svr_id) ;

	//����client��Ϣ����svr
	virtual void OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) ;

	//����client��Ϣ��.������֤�İ�
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) ;

	//client����֪ͨ
	virtual void OnClientDisCon(const SessionId &id) ;

	//client���룬�����Ự�� �������� ��socket���ӿͻ���
	virtual void OnClientConnect(const SessionId &id) ;

	//@id �������һ��
	//@main_cmd �������һ��
	//@svr_id 0 ��ʾʧ�ܡ�
	//�ο� SetMainCmd2Svr
	virtual void OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id) ;
};


class HearBeatClient : public BaseClient
{
public:
	enum class State
	{
		WAIT_VERFIY_RSP, //send verify req, wait rsp
		WAIT_BEAT_RSP,//send heartbeat, wait rsp heartbeat
		WAIT_DISCONNECTED,//stop send heartbeat, wait acc disconnect.
		END,
	};

	BaseFunFollowMgr &m_mgr;
	State m_state;

public:
	HearBeatClient(BaseFunFollowMgr &mgr);
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;

};

// svr���Ӷ����Ǹ��� BaseFlowSvr������
class HearBeatSvr : public ISvrCallBack
{
public:
	enum class State
	{
		WAIT_VERFIY_REQ, 
		WAIT_CLIENT_BEAT_TIME_OUT, //set beathear info , start client connect, wait client beat
		END,
	};

	BaseFunFollowMgr &m_mgr;
	State m_state;

public:
	HearBeatSvr(BaseFunFollowMgr &mgr);
	void Start();
	//�ص�ע����, ʧ�ܾ������ô����ˣ��޷��޸����������̰ɡ�
	//@svr_id = 0��ʾʧ��
	virtual void OnRegResult(uint16 svr_id);

	//����client��Ϣ����svr
	virtual void OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

	//����client��Ϣ��.������֤�İ�
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len);

	//client����֪ͨ
	virtual void OnClientDisCon(const SessionId &id);

	//client���룬�����Ự�� �������� ��socket���ӿͻ���
	virtual void OnClientConnect(const SessionId &id);

	//@id �������һ��
	//@main_cmd �������һ��
	//@svr_id 0 ��ʾʧ�ܡ�
	//�ο� SetMainCmd2Svr
	virtual void OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id);
};

class BaseFunFollowMgr 
{
public:
	enum class State
	{
		RUN_CORRECT_FOLLOW,
		RUN_HEARBEAT,
		END,
	};

	//step 1  RUN_CORRECT_FOLLOW
	BaseFlowClient m_client;
	BaseFlowSvr m_svr;

	//step 2 ��������
	HearBeatClient m_h_client;
	HearBeatSvr m_h_svr;

	State m_state;
	lc::Timer m_tm;
public:
	BaseFunFollowMgr();
	bool Init();
	void StartHeartbeatTest();
	void CheckBearHeatEnd();

};
