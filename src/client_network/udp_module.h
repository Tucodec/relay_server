#ifndef UDP_MODULE_H
#define UDP_MODULE_H
#include <unordered_map>
#include <simple_uv/UDPClient.h>
#include <relay_server_sdk/client_udpserver_msg.h>
#include <relay_server_sdk/client_tcpserver_msg.h>
#include "client_thread_msg.h"

#define BEGIN_UDPCLIENT_BIND(MEDIA_DATA,MSG_FUNC) virtual int  ParsePacket(const UdpPacket& packet, const char* buf,int len,const sockaddr* srcAddr) \
	{ \
	if(packet.type<MEDIA_DATA){\
		MSG_FUNC(packet,buf,len,srcAddr); \
	}else\
	switch(packet.type)\
		{

#define UDPCLIENT_BIND(MSG_TYPE, MSG_FUNC) \
			case MSG_TYPE : { \
				MSG_FUNC(packet,buf,len,srcAddr);\
				break; \
				} 

#define END_UDPCLIENT_BIND \
		}\
			return -1; \
	}

class CTimerManager;
enum class P2PStatus { pending, LAN_success, WAN_success ,requireRelay };
struct ClientCtx {
	sockaddr_in WANAddr;
	sockaddr_in LANAddr;
	P2PStatus status = P2PStatus::pending;
};
// 登陆udp服务器，发送/接受媒体数据，维护用户列表
class CUDPModule : public CUDPClient {
	uint32_t m_nUserId;
	uint32_t m_nSessionId;
	std::shared_ptr<CTimerManager> m_timerManager;
	std::unordered_map<int, ClientCtx>m_mapPeerClientCtx;
	bool m_bLoginSuccess;
	bool m_bEnableP2P;
	uv_timer_t m_timerKeepServerConnect;

	void OnRecvLoginAck(const UdpPacket& packet, const char* buf, int len, const  sockaddr* srcAddr);
	void OnRecvMediaData(const UdpPacket& packet, const char* buf, int len, const  sockaddr* srcAddr);
	void OnRecvPunchHoleMsg1(const UdpPacket& packet, const char* buf, int len, const  sockaddr* srcAddr);
	void OnRecvPunchHoleMsg2(const UdpPacket& packet, const char* buf, int len, const  sockaddr* srcAddr);

	void OnUvThreadMessage(const CRspP2PAddrMsg &msg, unsigned int nSrcAddr);
	void OnUvThreadMessage(const CReqSendMediaMsg &msg, unsigned int nSrcAddr);
	void OnUvThreadMessage(const CReqUdpLoginMsg &msg, unsigned int nSrcAddr);
	void OnUvThreadMessage(const CReqRemoveForwardNodeMsg &msg, unsigned int nSrcAddr);
	void OnUvThreadMessage(const CReqLeaveRoomMsg &msg, unsigned int nSrcAddr);
	void OnUvThreadMessage(const CReqAddForwardNodeMsg &msg, unsigned int nSrcAddr);

	void tryLANP2PConnect(uint32_t targetUserId);
	void tryWANP2PConnect(uint32_t targetUserId);
	void AddTransmitNode(uint32_t targetUserId);
public:
	BEGIN_UDPCLIENT_BIND(MULTI_FORWARD_MSG, OnRecvMediaData)
		UDPCLIENT_BIND(ACK_LOGIN, OnRecvLoginAck)
		UDPCLIENT_BIND(PUNCH_HOLE_MSG1, OnRecvPunchHoleMsg1)
		UDPCLIENT_BIND(PUNCH_HOLE_MSG2, OnRecvPunchHoleMsg2)
	END_UDPCLIENT_BIND

	BEGIN_UV_THREAD_BIND
		//UV_THREAD_BIND(CRspP2PAddrMsg::MSG_ID, CRspP2PAddrMsg)
		UV_THREAD_BIND(CReqSendMediaMsg::MSG_ID, CReqSendMediaMsg)
		UV_THREAD_BIND(CReqUdpLoginMsg::MSG_ID, CReqUdpLoginMsg)
		UV_THREAD_BIND(CReqRemoveForwardNodeMsg::MSG_ID, CReqRemoveForwardNodeMsg)
		UV_THREAD_BIND(CReqLeaveRoomMsg::MSG_ID, CReqLeaveRoomMsg)
		UV_THREAD_BIND(CReqAddForwardNodeMsg::MSG_ID, CReqAddForwardNodeMsg)
	END_UV_THREAD_BIND(CUDPClient)
	CUDPModule(uint32_t userId,uint32_t sessionId,uv_loop_t*pLoop);
	virtual ~CUDPModule();
	friend class CTimerManager;
	friend class ClientNetwork;
	bool LoginRelayServer();
	void SendMedia(char *base, int len);
	bool Connect(const char*ip,int port);
	bool InRoom(uint32_t targetUserId);
};
#endif  // UDP_MODULE_H