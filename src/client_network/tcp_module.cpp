
#include "network_module.h"
#include "tcp_module.h"
#include "udp_module.h"
#include "simple_uv/log4z.h"
#include <simple_uv/common.h>
CTCPModule::CTCPModule(uint32_t userId):
	m_bGetAddrSuccess(false),
	m_nUserId(userId)
{

}


CTCPModule::~CTCPModule()
{
}

int CTCPModule::requestRelayServerAddr(uint32_t sessionId)
{
	CReqSlaveAddrMsg msg;
	msg.sessionId = sessionId;
	this->SendUvMessage(msg, msg.MSG_ID);
	int try_time = 5;
	while (!m_bGetAddrSuccess && try_time-- >0) {
		uv_thread_sleep(500);
	}
	return m_bGetAddrSuccess ? 0 : -1;
}

void CTCPModule::bindUserIdOnServer()
{
	CReqBindUserId msg;
	msg.userId = m_nUserId;
	this->SendUvMessage(msg, msg.MSG_ID);
}

void CTCPModule::AddForwardNode(uint32_t targetUserId)
{
	CReqUpdateForwardTableMsg msg;
	msg.add = 1;
	msg.dstUserId = targetUserId;
	msg.srcUserId = m_nUserId;
	this->SendUvMessage(msg, msg.MSG_ID);
}

void CTCPModule::RemoveForwardNode(uint32_t targetUserId)
{
	if (m_nUserId == targetUserId)
		return;
	CReqUpdateForwardTableMsg msg;
	msg.add = 0;
	msg.dstUserId = targetUserId;
	msg.srcUserId = m_nUserId;
	this->SendUvMessage(msg, msg.MSG_ID);
}

//void CTCPModule::Logout()
//{
//	CReqLogoutMsg msg;
//	msg.userId;
//	this->SendUvMessage(msg, msg.MSG_ID);
//}

void CTCPModule::GetP2PAddr(uint32_t targetUserId)
{
	CReqP2PAddrMsg msg;
	msg.srcUserId=m_nUserId;
	msg.dstUserId=targetUserId;
	this->SendUvMessage(msg, msg.MSG_ID);
}

void CTCPModule::AfterReconnectSuccess() {
	this->bindUserIdOnServer();
	ClientNetwork *p = ClientNetwork::GetInstance();
	if (p->m_pNetworkChangeCB)
		p->m_pNetworkChangeCB(NetworkStatus::CONNECTED);
}

int CTCPModule::OnUvMessage(const CRspP2PAddrMsg & msg, TcpClientCtx *)
{
	PRINT_DEBUG("get p2p addr of uid:%d %s\n", msg.dstUserId,msg.success?"success":"fail");
	if(msg.success)
		ClientNetwork::AfterGetP2PAddr(msg);
	else {
		ClientNetwork *client=ClientNetwork::GetInstance();
		if (client->m_bInRoom == false || !client->m_pUdpClient->InRoom(msg.dstUserId))
			return 0;
		m_setFailRequest.insert(msg.dstUserId);
		uv_timer_t *timer = new uv_timer_t;
		timer->data = this;
		uv_timer_init(&m_loop, timer);
		uv_timer_start(timer, 
		[](uv_timer_t*timer) {
			CTCPModule *tcp = reinterpret_cast<CTCPModule*>(timer->data);
			CReqP2PAddrMsg msg;
			msg.srcUserId=tcp->m_nUserId;
			for (int dst : tcp->m_setFailRequest) {
				msg.dstUserId = dst;
				tcp->SendUvMessage(msg,msg.MSG_ID);
			}
			tcp->m_setFailRequest.clear();
			uv_timer_stop(timer);
			uv_close(reinterpret_cast<uv_handle_t*>(timer), [](uv_handle_t* handle) {free(handle); });
		}
		, 1000, 1000);
	}
	return 0;
}

void CTCPModule::AfterDisconnected()
{
	ClientNetwork *p = ClientNetwork::GetInstance();
	if (p->m_pNetworkChangeCB)
		p->m_pNetworkChangeCB(NetworkStatus::DISCONNECTED);
}

int CTCPModule::OnUvMessage(const CRspSlaveAddrMsg &msg, TcpClientCtx *pClient)
{
	PRINT_DEBUG("get relay server addr:%s:%u\n", ip_int2string(msg.addr).c_str(),msg.port);
	m_nRelayServerAddr = msg.addr;
	m_nRelayServerPort = msg.port;
	m_bGetAddrSuccess = true;
	return 0;
}
