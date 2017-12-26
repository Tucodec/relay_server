
#include <string>
#include <utility>
#include <simple_uv/common.h>
#include "udp_module.h"
#include "timer_module.h"
#include "network_module.h"

void CUDPModule::OnRecvLoginAck(const UdpPacket& packet, const char* buf, int len, const sockaddr* srcAddr) {
	m_bLoginSuccess = true;
}

void CUDPModule::OnRecvMediaData(const UdpPacket & packet, const char * buf, int len, const  sockaddr * srcAddr)
{
	uint32_t userid = *reinterpret_cast<const uint32_t*>(buf);
	if (m_mapPeerClientCtx.find(userid) == m_mapPeerClientCtx.end())
		return;
	buf += 4;
	len -= 4;
	//PRINT_DEBUG("receive media data\n");
	ClientNetwork*clientNetwork = ClientNetwork::GetInstance();
	if (clientNetwork->m_pVoipDataListener)
		clientNetwork->m_pVoipDataListener->VoipDataReceive(const_cast<char*>(buf), len, userid);
}

void CUDPModule::OnRecvPunchHoleMsg1(const UdpPacket & packet, const char * buf, int len, const sockaddr * srcAddr)
{
	PRINT_DEBUG("recv uid %d's punchhole msg,addr%s\n", packet.userId, inet_ntoa(((sockaddr_in*)srcAddr)->sin_addr));
	if (packet.dstUserId != m_nUserId)
		return;
	UdpPacket msg=packet;
	msg.type = PUNCH_HOLE_MSG2;
	CUDPHandle::send( reinterpret_cast<char*>(&msg),sizeof(UdpPacket) , reinterpret_cast<const sockaddr_in*>( srcAddr));
}

void CUDPModule::OnRecvPunchHoleMsg2(const UdpPacket & packet, const char * buf, int len, const sockaddr * srcAddr)
{
	PRINT_DEBUG("punch hole success from addr:%s\n", inet_ntoa(((sockaddr_in*)srcAddr)->sin_addr));
	m_timerManager->removeP2PTimer(packet.dstUserId);
	auto it = m_mapPeerClientCtx.find(packet.dstUserId);
	if (it == m_mapPeerClientCtx.end()) {
		PRINT_DEBUG("error happened on punch hole.uid %u punch hole success without request\n",packet.userId);
		return;
	}
	if (packet.del == 0) {//LAN P2P SUCCESS
		it->second.status = P2PStatus::LAN_success;
		it->second.LANAddr = *reinterpret_cast<const sockaddr_in*> (srcAddr);
	}
	else {//WAN P2P SUCCESS
		if (memcmp(reinterpret_cast<const void*>(srcAddr), reinterpret_cast<void*>(&it->second.WANAddr),sizeof(sockaddr_in)) != 0) {
			PRINT_DEBUG("unknown status happened on punch hole success\n");
		}
		it->second.status = P2PStatus::WAN_success;
	}
}

CUDPModule::CUDPModule(uint32_t userId, uint32_t sessionId ,uv_loop_t* pLoop) :
	CUDPClient(pLoop),
	m_nUserId(userId),
	m_nSessionId(sessionId),
	m_bLoginSuccess(false),
	m_bEnableP2P(true)
{
	m_timerManager =std::make_shared<CTimerManager>(this);
}

CUDPModule::~CUDPModule()
{
	m_timerManager.reset();
}

bool CUDPModule::LoginRelayServer()
{
	m_bLoginSuccess = false;
	CReqUdpLoginMsg msg;
	this->SendUvThreadMessage(msg, msg.MSG_ID);
	int try_times = 5;
	while (!m_bLoginSuccess&&try_times-->0)
		uv_thread_sleep(500);
	return m_bLoginSuccess;
}

void CUDPModule::SendMedia(char * base, int len)
{
	CReqSendMediaMsg msg;
	msg.base = base;
	msg.len = len;
	this->SendUvThreadMessage(msg, msg.MSG_ID);
}

bool CUDPModule::Connect(const char *ip,int port)
{
	if (!CUDPClient::Connect(ip,port)) {
		return false;
	}
	return setBroadcast(1);
}

bool CUDPModule::InRoom(uint32_t targetUserId)
{
	return m_mapPeerClientCtx.find(targetUserId) != m_mapPeerClientCtx.end();
}

void CUDPModule::OnUvThreadMessage(const CRspP2PAddrMsg &msg, unsigned int nSrcAddr)
{
	auto it = m_mapPeerClientCtx.find(msg.dstUserId);
	if (it == m_mapPeerClientCtx.end()) {
		PRINT_DEBUG("unexpected P2P addr response,uid:%u\n",msg.dstUserId);
		return;
	}
	std::string str = ip_int2string(msg.dstIp);
	//inet_ntoa(*reinterpret_cast<const in_addr*>(&msg.dstIp));
	const char *ip = str.c_str();
	int WANUdpPort = ntohs(msg.dstPort);
	int LANUdpPort = ntohs(msg.dstLocalUdpPort);
	PRINT_DEBUG("uid %d p2p addr:%s:%d,localUdpPort:%u\n", msg.dstUserId, ip, WANUdpPort, LANUdpPort);
	uv_ip4_addr(ip, WANUdpPort, &it->second.WANAddr);
	uv_ip4_addr("255.255.255.255", LANUdpPort, &it->second.LANAddr);
	tryLANP2PConnect(msg.dstUserId);
}

void CUDPModule::OnUvThreadMessage(const CReqSendMediaMsg &msg, unsigned int nSrcAddr)
{
#ifdef WIN32
	if (!m_bEnableP2P) {
		send(msg.base, msg.len);
		free(msg.base);
		return;
	}
	bool requireRelay = false;
	for (auto &&ctx : m_mapPeerClientCtx) {
		if (ctx.second.status == P2PStatus::WAN_success)
			CUDPHandle::send(msg.base, msg.len, &ctx.second.WANAddr);
		else if (ctx.second.status == P2PStatus::LAN_success)
			CUDPHandle::send(msg.base, msg.len, &ctx.second.LANAddr);
		else //if(ctx.second.status==P2PStatus::requireRelay)
			requireRelay = true;
	}
	if (requireRelay)
		send(msg.base, msg.len);
	free(msg.base);
#else
	struct msghdr msg2;
	struct iovec iov;

	msg2.msg_iov = &iov;
	msg2.msg_iovlen = 1;
	msg2.msg_iov->iov_base = msg.base;
	msg2.msg_iov->iov_len = msg.len;
	msg2.msg_control = 0;
	msg2.msg_controllen = 0;
	msg2.msg_flags = 0;
	msg2.msg_namelen = sizeof(sockaddr_in);

	if (!m_bEnableP2P) {
		msg2.msg_name = &m_serverAddr;
		sendmsg(this->m_udpHandle.io_watcher.fd, &msg2, 0);
		free(msg.base);
		return;
	}
	bool requireRelay = false;
	for (auto &&ctx : m_mapPeerClientCtx) {
		if (ctx.second.status == P2PStatus::WAN_success) {
			msg2.msg_name = &ctx.second.WANAddr;
			sendmsg(this->m_udpHandle.io_watcher.fd, &msg2, 0);
		}
		else if (ctx.second.status == P2PStatus::LAN_success) {
			msg2.msg_name = &ctx.second.LANAddr;
			sendmsg(this->m_udpHandle.io_watcher.fd, &msg2, 0);
		}
		else //if(ctx.second.status==P2PStatus::requireRelay)
			requireRelay = true;
}
	if (requireRelay) {
		msg2.msg_name = &m_serverAddr;
		sendmsg(this->m_udpHandle.io_watcher.fd, &msg2, 0);
	}
	free(msg.base);
#endif
}

void CUDPModule::OnUvThreadMessage(const CReqUdpLoginMsg &msg, unsigned int nSrcAddr)
{
	sockaddr_storage addr;
	int len = sizeof(addr);
	uv_udp_getsockname(&this->m_udpHandle, (sockaddr*)&addr, &len);
	UdpPacket sendMsg;
	sendMsg.userId = m_nUserId;
	sendMsg.type = LOGIN_MSG;
	sendMsg.sessionId = m_nSessionId;
	sendMsg.localPort = reinterpret_cast<sockaddr_in*>(&addr)->sin_port;
	PRINT_DEBUG("local port:%u\n",ntohs(sendMsg.localPort));
	m_timerManager->gernerateLoginTimer(&sendMsg,0,300);  // repeat time = 0 -> repeat 2147483647 
}

void CUDPModule::OnUvThreadMessage(const CReqRemoveForwardNodeMsg &msg, unsigned int nSrcAddr)
{
	m_mapPeerClientCtx.erase(msg.targetUserId);
	m_timerManager->removeP2PTimer(msg.targetUserId);
}

void CUDPModule::OnUvThreadMessage(const CReqLeaveRoomMsg &msg, unsigned int nSrcAddr)
{
	m_mapPeerClientCtx.clear();
	m_timerManager->clearP2PTimer();
	ClientNetwork::GetInstance()->m_bInRoom = false;
}

void CUDPModule::OnUvThreadMessage(const CReqAddForwardNodeMsg & msg, unsigned int nSrcAddr)
{
	if (m_mapPeerClientCtx.find(msg.targetUserId) != m_mapPeerClientCtx.end())
		return;  // error: add duplicate user 
	ClientCtx ctx;
	if (!m_bEnableP2P) 
		ctx.status = P2PStatus::requireRelay;
	m_mapPeerClientCtx.insert(std::make_pair(msg.targetUserId, ctx));

	if (!m_bEnableP2P)
		ClientNetwork::AfterP2PFail(msg.targetUserId);
	else
		ClientNetwork::TryP2PConnect(msg.targetUserId);
}

void CUDPModule::tryLANP2PConnect(uint32_t targetUserId)
{
	UdpPacket punchhole_msg;
	punchhole_msg.userId = m_nUserId;
	punchhole_msg.dstUserId = targetUserId;
	punchhole_msg.type = PUNCH_HOLE_MSG1;
	punchhole_msg.del = 0;//LAN P2P
	m_timerManager->removeP2PTimer(targetUserId);
	m_timerManager->gernerateP2PTimer(&punchhole_msg,4,25);
}

void CUDPModule::tryWANP2PConnect(uint32_t targetUserId)
{
	UdpPacket punchhole_msg;
	punchhole_msg.userId = m_nUserId;
	punchhole_msg.dstUserId = targetUserId;
	punchhole_msg.type = PUNCH_HOLE_MSG1;
	punchhole_msg.del = 1;//WAN P2P
	m_timerManager->removeP2PTimer(targetUserId);
	m_timerManager->gernerateP2PTimer(&punchhole_msg,10,150);
}

void CUDPModule::AddTransmitNode(uint32_t targetUserId)
{
	CReqAddForwardNodeMsg msg;
	msg.targetUserId = targetUserId;
	this->SendUvThreadMessage(msg, msg.MSG_ID);
}
