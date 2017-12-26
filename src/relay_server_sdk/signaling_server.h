#ifndef SIGNALING_SERVER_H
#define SIGNALING_SERVER_H
#include <unordered_map>
#include <simple_uv/tcpserver.h>
#include "client_tcpserver_msg.h"
class CMediaServer;
// tcp信令服务器：处理转发表的修改请求、协调P2P
class CSignalingServer :
	public CTCPServer
{
public:
	CSignalingServer();
	void setUdpServer(CMediaServer *udpServer);
	virtual ~CSignalingServer();	

protected:
	BEGIN_UV_BIND
		UV_BIND(CReqUpdateForwardTableMsg::MSG_ID, CReqUpdateForwardTableMsg)
		UV_BIND(CReqLogoutMsg::MSG_ID, CReqLogoutMsg)
		UV_BIND(CReqBindUserId::MSG_ID, CReqBindUserId)
		UV_BIND(CReqP2PAddrMsg::MSG_ID, CReqP2PAddrMsg)
		UV_BIND(CReqLeaveRoomMsg::MSG_ID, CReqLeaveRoomMsg)
	END_UV_BIND(CTCPServer)

	int OnUvMessage(const CReqUpdateForwardTableMsg &msg, TcpClientCtx *pClient);
	int OnUvMessage(const CReqLogoutMsg &msg, TcpClientCtx *pClient);
	int OnUvMessage(const CReqBindUserId &msg, TcpClientCtx *pClient);
	int OnUvMessage(const CReqP2PAddrMsg &msg, TcpClientCtx *pClient);
	int OnUvMessage(const CReqLeaveRoomMsg &msg, TcpClientCtx *pClient);

	void OnUvThreadMessage(const CRspP2PAddrMsg &msg, unsigned int nSrcAddr);

	virtual void CloseCB(int clientid);
	void OnUserTimeout(uint32_t userId);
private:
	struct TcpClientCtxEx {
		TcpClientCtx*tcpClientCtx;
		int userId;
	};
	unordered_map<int, TcpClientCtxEx>m_mapClientId;  // <key,value>=<clientId,userId>
	unordered_map<uint32_t, int>m_mapUserId;  // <key,value>=<userId,clientId>
	CMediaServer *m_pUdpServer;
	friend class CMediaServer;
};
#endif  // SIGNALING_SERVER_H