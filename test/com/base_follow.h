/*
	�ɸ��ò���client svr.
	acc_driver.h ÿ�����ܵĴ����һ�顣
	ĳЩ�������Ӳ�������
*/
#pragma once
#include <string>
#include <array>
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



//RUN_BROADCAST_DISCON client
class BDClient : public BaseClient
{
public:
	enum class State
	{
		END,
	};

	BaseFunFollowMgr &m_mgr;
	State m_state;
	uint32 m_id; //id,0��ʼ����������

public:
	BDClient(BaseFunFollowMgr &mgr);
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;
};

//RUN_BROADCAST_DISCON svr
class BDSvr : public ISvrCallBack
{
public:
	enum class State
	{
		WAIT_ALL_CLIENT_CONNECT, //
		WAIT_ALL_VERIFY_OK,
		WAIT_BROADCAST, //broadcast all, broadcast one client,
		WAIT_DISCON_ONE,//discon one client
		WAIT_DISCON_ALL, //, discon all client.
		END,
	};

	BaseFunFollowMgr &m_mgr;
	State m_state;
	std::set<uint64> m_client_set; //ͳ��client����
	uint32 m_broadCmd_cnt;
	uint32 m_broadpartCmd_cnt;
	SessionId m_anyone_sid;
	uint32 m_tmp_num;
public:
	BDSvr(BaseFunFollowMgr &mgr);
	void Start();
	void ClientOnConnected(uint32 idx);
	void ClientRevBroadCmd();
	void ClientRevBroadPartCmd();
	void CheckBroadEnd();
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

//RUN_ROUTE
class RouteClient : public BaseClient
{
public:
	enum class State
	{
		WAIT_VERIFY_OK,
		WAIT_SEND_SVR1,//wait svr1 rev msg, svr1 change route 
		WAIT_SEND_SVR2,
		END,
	};

	BaseFunFollowMgr &m_mgr;
	State m_state;
public:
	RouteClient(BaseFunFollowMgr &mgr);
	virtual void OnRecvMsg(uint32 cmd, const std::string &msg) override final;
	virtual void OnConnected() override final;
	virtual void OnDisconnected() override final;
};

//RUN_ROUTE
class RouteSvr : public ISvrCallBack
{
public:
	enum class State
	{
		END,
	};

	BaseFunFollowMgr &m_mgr;
	State m_state;
public:
	RouteSvr(BaseFunFollowMgr &mgr);
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
		RUN_BROADCAST_DISCON, //���Թ㲥������
		RUN_ROUTE,//·������
		END,
	};

	//step 1  RUN_CORRECT_FOLLOW
	BaseFlowClient m_client;
	BaseFlowSvr m_svr; //����AllADFacadeMgr::Obj()��״̬������ı�Ϊ��Ч��

	//step 2 ��������
	HearBeatClient m_h_client;
	HearBeatSvr m_h_svr;

	//step3 broad cast dicon client
	std::array<BDClient,3> m_bd_client;
	BDSvr m_bd_svr;


	//step4 test route
	RouteSvr m_route_svr1;
	RouteSvr m_route_svr2;
	RouteClient m_route_client;

	State m_state;
	lc::Timer m_tm;
	AllADFacadeMgr m_svr1;
	AllADFacadeMgr m_svr2;
public:
	BaseFunFollowMgr();
	bool Init();
	void StartHeartbeatTest();
	void CheckBearHeatEnd();
	void StartCheckBD();//TEST RUN_BROADCAST_DISCON
	void StartCheckRoute();//TEST RUN_ROUTE

};
