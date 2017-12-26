#include "signaling_server.h"
#include "media_server.h"

CSignalingServer::CSignalingServer():
	m_pUdpServer(nullptr)
{
}

void CSignalingServer::setUdpServer(CMediaServer * udpServer)
{
	m_pUdpServer = udpServer;
}

CSignalingServer::~CSignalingServer()
{
}

int CSignalingServer::OnUvMessage(const CReqLogoutMsg & msg, TcpClientCtx * pClient)
{
	CloseCB(pClient->clientid);
	return 0;
}

int CSignalingServer::OnUvMessage(const CReqBindUserId & msg, TcpClientCtx * pClient)
{
	LOGFMTI("recv bind userId msg,userId %u clientId %u\n",msg.userId,pClient->clientid);
	TcpClientCtxEx ctx = {pClient,msg.userId};  // ctx.userId = msg.userId ; ctx.tcpClientCtx = pClient;
	m_mapClientId.insert( make_pair( pClient->clientid, ctx ) );
	return 0;
}

int CSignalingServer::OnUvMessage(const CReqP2PAddrMsg & msg, TcpClientCtx * pClient)
{
	LOGFMTI("recv req p2p addr msg,from %u to %u\n",msg.srcUserId,msg.dstUserId);
	CReqP2PAddrMsg copy = msg;
	copy.clientId = pClient->clientid;
	m_pUdpServer->OnUvThreadMessage(copy, 0);
	return 0;
}

int CSignalingServer::OnUvMessage(const CReqLeaveRoomMsg & msg, TcpClientCtx * pClient)
{
	LOGFMTI("receive leave room,uid %u\n", msg.userId);
	m_pUdpServer->OnUvThreadMessage(msg, 0);
	return 0;
}

int CSignalingServer::OnUvMessage(const CReqUpdateForwardTableMsg & msg, TcpClientCtx * pClient)
{
	LOGFMTI("receive update forwardtable,%u %s %u\n",msg.srcUserId,msg.add?"add":"del",msg.dstUserId);
	m_pUdpServer->OnUvThreadMessage(msg, 0);
	return 0;
}

void CSignalingServer::OnUvThreadMessage(const CRspP2PAddrMsg &msg, unsigned int nSrcAddr)
{
	auto it = m_mapClientId.find(msg.clientId);
	if (it == m_mapClientId.end()) {
		LOGFMTI("can't find client id %d when rsp p2p addr\n",msg.clientId);
		return ;
	}
	this->SendUvMessage(msg, msg.MSG_ID, it->second.tcpClientCtx);
}

void CSignalingServer::CloseCB(int clientId)
{
	auto it = m_mapClientId.find(clientId);
	if (it != m_mapClientId.end()) {
		LOGFMTI("client id %d,uid %d disconnected\n", clientId, it->second.userId);
		CReqLogoutMsg msg;
		msg.userId=it->second.userId;
		m_pUdpServer->OnUvThreadMessage(msg, 0);
		m_mapClientId.erase(it);
	}
}

void CSignalingServer::OnUserTimeout(uint32_t userId)
{
	// use userId to search clientId, then delete them
	auto it1 = m_mapUserId.find(userId);
	if (it1 == m_mapUserId.end()) {
		return;
	}
	int clientId = it1->second;
	m_mapUserId.erase(it1);

	m_mapClientId.erase(clientId);
	auto it2 = m_mapClientsList.find(clientId);
	it2->second->Close();
}
