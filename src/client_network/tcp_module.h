#ifndef TCP_MODULE_H
#define TCP_MODULE_H
#include <unordered_set>
#include <simple_uv/tcpclient.h>
#include <relay_server_sdk/client_tcpserver_msg.h>
// 接受/发送tcp消息，例如操纵转发表、请求P2P
class CTCPModule :
	public CTCPClient
{
	bool m_bGetAddrSuccess;
	const uint32_t m_nUserId;
public:
	uint32_t m_nRelayServerAddr;
	uint16_t m_nRelayServerPort;
	std::unordered_set<uint32_t>m_setFailRequest;  // now only for failed get p2p addr request
	CTCPModule(uint32_t userId);
	virtual ~CTCPModule();
	int requestRelayServerAddr(uint32_t sessionId);
	void bindUserIdOnServer();
	void AddForwardNode(uint32_t targetUserId);
	void RemoveForwardNode(uint32_t targetUserId);
	//void Logout();
	void GetP2PAddr(uint32_t targetUserId);
protected:
	BEGIN_UV_BIND
		UV_BIND(CRspSlaveAddrMsg::MSG_ID, CRspSlaveAddrMsg)
		UV_BIND(CRspP2PAddrMsg::MSG_ID, CRspP2PAddrMsg)
	END_UV_BIND(CTCPClient)

	virtual void AfterReconnectSuccess();
	virtual void AfterDisconnected();
	int OnUvMessage(const CRspSlaveAddrMsg &msg, TcpClientCtx*);
	int OnUvMessage(const CRspP2PAddrMsg &msg, TcpClientCtx*);
};
#endif  // TCP_MODULE_H