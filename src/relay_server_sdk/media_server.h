#ifndef TURNSERVER_H
#define TURNSERVER_H
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "simple_uv/UDPServer.h"
#include "client_udpserver_msg.h"
#include "relay_server.h"
#include "slave.h"

#define BEGIN_UDPUV_BIND(MEDIA_DATA,MSG_FUNC) virtual int  ParsePacket(const UdpPacket& packet, const char* buf,int len,const sockaddr* srcAddr) \
	{ \
	if(packet.type<MEDIA_DATA){\
		MSG_FUNC(packet,buf,len,srcAddr); \
	}else\
	switch(packet.type)\
		{\

#define UDPUV_BIND(MSG_TYPE, MSG_FUNC) \
			case MSG_TYPE : { \
				MSG_FUNC(packet,buf,len,srcAddr);\
				break; \
				} \

#define END_UDPUV_BIND \
		}\
			return -1; \
	} 

class CSignalingServer;
// udp服务器：处理媒体数据的转发
class SUV_EXPORT CMediaServer :
	public CUDPServer
{
public:
	CMediaServer(uv_loop_t *pLoop, CRelayServer* httpClient=NULL);
	bool Start(const char* ip=NULL, int port=12345);
	//void setValidateClass(HttpClient*);
	virtual ~CMediaServer();
	void setSignalingServer(CSignalingServer *theclass);
	void setServerIPAndPort(const std::string &ip, int port);
	void setMasterIPAndPort(const std::string &ip, int port);
	void setValidUrl(const std::string &url);
	void setLoginValidUrl(const std::string &url);
protected:
	void OnUvThreadMessage(const CReqUpdateForwardTableMsg &msg, unsigned int nSrcAddr);
	void OnUvThreadMessage(const CReqP2PAddrMsg &msg, unsigned int nSrcAddr);
	void OnUvThreadMessage(const CReqLogoutMsg &msg, unsigned int nSrcAddr);
	void OnUvThreadMessage(const CReqLeaveRoomMsg &msg, unsigned int nSrcAddr);

	BEGIN_UDPUV_BIND(MULTI_FORWARD_MSG, OnRecvMultiForwardMsg)
		UDPUV_BIND(LOGIN_MSG, OnRecvLoginMsg)
		//UDPUV_BIND(UPDATE_FORWARD_TABLE_MSG, OnRecvUpdateForwardTableMsg)
		UDPUV_BIND(SINGLE_FORWARD_MSG, OnRecvSingleForwardMsg)
		UDPUV_BIND(LOGOUT_MSG, OnRecvLogoutMsg)
	END_UDPUV_BIND

    /* deprecated : 
    *  Used for test. Now operation for forwardtable is transmit with TCP protocol
    *  int OnRecvUpdateForwardTableMsg(const UdpPacket& msg, const char* buf, int len, const sockaddr* srcAddr);
	*  int OnUpdateForwardInfo(const UdpPacket* msg, const sockaddr* srcAddr);
    */
	int OnRecvLoginMsg(const UdpPacket& msg, const char* buf, int len,const sockaddr* srcAddr);
	int OnRecvSingleForwardMsg(const UdpPacket& msg, const char* buf, int len, const sockaddr* srcAddr);
	int OnRecvMultiForwardMsg(const UdpPacket& msg, const char* buf, int len, const sockaddr* srcAddr);
	int OnRecvLogoutMsg(const UdpPacket& msg, const char* buf, int len, const sockaddr* srcAddr);

	int OnUpdateLoginInfo(const UdpPacket* msg, const sockaddr* srcAddr);
	int OnLogout(int userId);
	void OnAckClient(const int msg,const UdpPacket*packet,const sockaddr* srcAddr);
	inline void OnUpdateActiveTime(int userId);

	static void AfterVerification(uv_work_t* req, int status);
	static void ID_Verification(uv_work_t*req);
	static void offlineDetection(uv_timer_t* timer);
	static void statusDetection(uv_timer_t *timer);
	const uint64_t m_nTimeout = 30000;
	const uint64_t m_nStatusReportTime = 30000;
	friend class CSignalingServer;
private:
	typedef struct forwardTable {
		sockaddr_in addr;  // user's public addr
		uint32_t sessionId;
		uint64_t expireTime;
		uint16_t localUdpPort;  // used for LAN P2P
		std::unordered_map<unsigned int, sockaddr_in>dataTo;  // (key,value)=(userId,sockaddr_in)
		std::unordered_set<unsigned int>dataFrom;  // reverse search,key=userId
	}ForwardTable;

	std::unordered_map<unsigned int, ForwardTable >m_mapForwardTable;

	CRelayServer *m_pHttpClient;
	CSignalingServer *m_pSignalServer;

	uv_timer_t m_OfflineDetectionTimer;
	uv_timer_t m_StatusReportTimer;

	int m_nMasterPort;

	std::shared_ptr<CSlave>m_pTcpClient;

	std::string m_strMasterIp;
	std::string m_strValidateUrl;
	std::string m_strValidateUrlOnLogin;
};
typedef struct _VerifyCtx {
	CMediaServer *server;
	UdpPacket *msg;
	sockaddr* srcAddr;
	bool success;
}VerifyCtx;
VerifyCtx* InitAndAllocVerifyCtx(CMediaServer *server, UdpPacket *msg,const sockaddr* srcAddr);
void FreeVerifyCtx(VerifyCtx* ctx);

#endif  // TURNSERVER_H