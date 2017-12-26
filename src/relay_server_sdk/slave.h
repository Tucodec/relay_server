#pragma once
#include "simple_uv/tcpclient.h"
#include "client_tcpserver_msg.h"
// 转发服务器从节点与主节点通信用套接字类
class CSlave :
	public CTCPClient
{
	uint32_t m_nSlaveServerIp;
	uint32_t m_nSlaveServerPort;
public:
	CSlave(uint32_t ip, uint16_t port);
	virtual ~CSlave();
	int LoginAsSlave();

protected:
	BEGIN_UV_BIND
		//		UV_BIND(CSlaveLoginMsg::MSG_ID,CSlaveLoginMsg)
		END_UV_BIND(CTCPClient)

		//int OnUvMessage(const CSlaveLoginMsg &msg, TcpClientCtx *pClient);

	virtual void AfterReconnectSuccess();
	virtual void AfterDisconnected();
};