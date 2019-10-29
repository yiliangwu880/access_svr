/*
	�ɸ��ò���client svr
*/
#pragma once
#include <string>
#include "libevent_cpp/include/include_all.h"
#include "svr_util/include/su_mgr.h"
#include "svr_util/include/single_progress.h"
#include "svr_util/include/read_cfg.h"
#include "unit_test.h"
#include "../acc_driver/include/acc_driver.h"

using namespace su;
using namespace acc;
using namespace lc;


class ISvrCallBack
{
public:
	//�ص�ע����, ʧ�ܾ������ô����ˣ��޷��޸����������̰ɡ�
	//@svr_id = 0��ʾʧ��
	virtual void OnRegResult(uint16 svr_id) = 0;

	//����client��Ϣ����svr
	virtual void OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) = 0;

	//����client��Ϣ��.������֤�İ�
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) = 0;

	//client����֪ͨ
	virtual void OnClientDisCon(const SessionId &id) = 0;

	//client���룬�����Ự�� �������� ��socket���ӿͻ���
	virtual void OnClientConnect(const SessionId &id) = 0;

	//@id �������һ��
	//@main_cmd �������һ��
	//@svr_id 0 ��ʾʧ�ܡ�
	//�ο� SetMainCmd2Svr
	virtual void OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id) = 0;
};

class AllADFacadeMgr : public acc::ADFacadeMgr , public Singleton<AllADFacadeMgr>
{
public:
	ISvrCallBack *m_svr_cb;//��Ϊ ADFacadeMgr������һ��������Ҫ������Ϊ���ͼ��һ��.

	//�ص�ע����, ʧ�ܾ������ô����ˣ��޷��޸����������̰ɡ�
	//@svr_id = 0��ʾʧ��
	virtual void OnRegResult(uint16 svr_id) {
		m_svr_cb->OnRegResult(svr_id);
	}

	//����client��Ϣ����svr
	virtual void OnRevClientMsg(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) {
		m_svr_cb->OnRevClientMsg(id,cmd, msg, msg_len);
	}

	//����client��Ϣ��.������֤�İ�
	virtual void OnRevVerifyReq(const SessionId &id, uint32 cmd, const char *msg, uint16 msg_len) {
		m_svr_cb->OnRevVerifyReq(id, cmd, msg, msg_len);
	}

	//client����֪ͨ
	virtual void OnClientDisCon(const SessionId &id) {
		m_svr_cb->OnClientDisCon(id);
	}

	//client���룬�����Ự�� �������� ��socket���ӿͻ���
	virtual void OnClientConnect(const SessionId &id) {
		m_svr_cb->OnClientConnect(id);
	}

	//@id �������һ��
	//@main_cmd �������һ��
	//@svr_id 0 ��ʾʧ�ܡ�
	//�ο� SetMainCmd2Svr
	virtual void OnSetMainCmd2SvrRsp(const SessionId &id, uint16 main_cmd, uint16 svr_id) {
		m_svr_cb->OnSetMainCmd2SvrRsp(id, main_cmd, svr_id);
	}
};


