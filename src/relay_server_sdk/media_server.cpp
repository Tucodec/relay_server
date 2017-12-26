
#include <string>
#include <simple_uv/config.h>
#include <simple_uv/common.h>
#include "media_server.h"
#include "signaling_server.h"
//type of addr1,addr2: sockaddr*
#define addr_verification(addr1,addr2) \
	if (memcmp((sockaddr_in*)addr1, addr2, sizeof(sockaddr_in)) != 0) { \
		LOGA("someone try to Spoofing server.");\
		return -1;\
	}

#define addr_verification(addr1,addr2) 
CMediaServer::CMediaServer(uv_loop_t *pLoop, CRelayServer* httpClient) :
	CUDPServer(pLoop),
	m_pHttpClient(httpClient)
{

}

CMediaServer::~CMediaServer() {

}
void CMediaServer::OnUvThreadMessage(const CReqP2PAddrMsg &msg, unsigned int nSrcAddr)
{
	auto it = m_mapForwardTable.find(msg.dstUserId);
	CRspP2PAddrMsg reply;

	reply.clientId = msg.clientId;
	reply.dstUserId=msg.dstUserId;

	if (it == m_mapForwardTable.end()) {
		reply.success = false;
		LOGFMTE("can't find target p2p addr\n");
		m_pSignalServer->OnUvThreadMessage(reply, 0);
		return;
	}

	reply.dstIp=*reinterpret_cast<int*>( &it->second.addr.sin_addr);
	reply.dstPort=it->second.addr.sin_port;
	reply.dstLocalUdpPort = it->second.localUdpPort;
	reply.success = true;

	m_pSignalServer->OnUvThreadMessage(reply,0);
}
void CMediaServer::OnUvThreadMessage(const CReqLogoutMsg &msg, unsigned int nSrcAddr)
{
	OnLogout(msg.userId);
}
void CMediaServer::OnUvThreadMessage(const CReqLeaveRoomMsg &msg, unsigned int nSrcAddr)
{
	auto srcForwardTable = m_mapForwardTable.find(msg.userId);
	if (srcForwardTable == m_mapForwardTable.end()) {
		LOGFMTE("uid %d try to leave room,but uid not exist\n",msg.userId);
		return;
	}
	LOGFMTI("uid %d leave room\n", msg.userId);
	for (auto &dst : srcForwardTable->second.dataFrom) {
		auto it = m_mapForwardTable.find(dst);
		if(it!=m_mapForwardTable.end())
			it->second.dataTo.erase(msg.userId);
	}
	srcForwardTable->second.dataFrom.clear();
	srcForwardTable->second.dataTo.clear();
}
bool CMediaServer::Start(const char* ip, int port) {
	{
		uv_timer_init(m_pLoop, &m_OfflineDetectionTimer);
		m_OfflineDetectionTimer.data = this;
		uv_timer_start(&m_OfflineDetectionTimer, offlineDetection, m_nTimeout, m_nTimeout);
	}
	while (m_strMasterIp != "") {
		m_pTcpClient = make_shared<CSlave>(ip_string2int(m_strServerIP), m_nServerPort);
		if (!m_pTcpClient->Connect(m_strMasterIp.c_str(), m_nMasterPort))
			break;
		m_pTcpClient->LoginAsSlave();
		uv_timer_init(m_pLoop, &m_StatusReportTimer);
		m_StatusReportTimer.data = this;
		uv_timer_start(&m_StatusReportTimer, statusDetection, m_nStatusReportTime, m_nStatusReportTime);
		break;
	}
	return CUDPServer::Start(ip, port);
}
void CMediaServer::offlineDetection(uv_timer_t *timer) {
	CMediaServer *server = (CMediaServer*)timer->data;
	uint64_t now = uv_now(server->m_pLoop);
	for (auto it = server->m_mapForwardTable.begin(); it != server->m_mapForwardTable.end();)
		if (now > it->second.expireTime) {
			LOGFMTI("userId %u timeout", it->first);
			for (int delUserId : it->second.dataFrom)
				if(delUserId != it->first)
					server->m_mapForwardTable[delUserId].dataTo.erase(it->first);
			for (auto delUserCtx : it->second.dataTo)
				if (delUserCtx.first != it->first)
					server->m_mapForwardTable[delUserCtx.first].dataFrom.erase(it->first);
			server->m_pSignalServer->OnUserTimeout(it->first);  // delete userId on signalServer
			server->m_mapForwardTable.erase(it++);  //delete userId on mediaServer
		}
		else
			++it;
}
void CMediaServer::statusDetection(uv_timer_t *timer) {
	CMediaServer *server = reinterpret_cast<CMediaServer*>(timer->data);
	CSlaveStatusReportMsg msg;
	msg.network_status=server->m_nSendByteCount+server->m_nRecvByteCount;
	server->m_nSendByteCount = 0;
	server->m_nRecvByteCount = 0;
	server->m_pTcpClient->SendUvMessage(msg, msg.MSG_ID);
}

void CMediaServer::OnUpdateActiveTime(int userId)
{
	m_mapForwardTable[userId].expireTime = uv_now(m_pLoop)+3*m_nTimeout;
}

void CMediaServer::setSignalingServer(CSignalingServer * theclass)
{
	m_pSignalServer = theclass;
}
void CMediaServer::setServerIPAndPort(const std::string & ip, int port)
{
	m_strServerIP = ip;
	m_nServerPort = port;
}
void CMediaServer::setMasterIPAndPort(const std::string & ip, int port)
{
	m_strMasterIp = ip;
	m_nMasterPort = port;
}
void CMediaServer::setValidUrl(const std::string & url)
{
	m_strValidateUrl = url;
}
void CMediaServer::setLoginValidUrl(const std::string & url)
{
	m_strValidateUrlOnLogin = url;
}
int CMediaServer::OnRecvLoginMsg(const UdpPacket& msg, const char* buf, int len, const sockaddr* srcAddr){
	auto it = m_mapForwardTable.find(msg.userId);
	// if it is the first time userId login || ( userId's addr changed && sessionId changed ) 
	if (it == m_mapForwardTable.end() ||
		(memcmp(&it->second.addr, srcAddr, sizeof(sockaddr)) != 0&& msg.sessionId != it->second.sessionId )
		) 
	{
		char *ip = inet_ntoa(((sockaddr_in*)srcAddr)->sin_addr);
		int port = ntohs(((sockaddr_in*)srcAddr)->sin_port);
		LOGFMTI("receive login,userId:%u,addr:%s:%u\n", msg.userId, ip, port);
		uv_work_t* req = (uv_work_t*)malloc(sizeof(uv_work_t));
		req->data = InitAndAllocVerifyCtx(this, (UdpPacket*)&msg, srcAddr);
		uv_queue_work(m_pLoop, req, ID_Verification, AfterVerification);
	}
	else {
		OnAckClient(ACK_LOGIN, &msg, srcAddr);
		OnUpdateActiveTime(msg.userId);
	}
	return 0;
}
int CMediaServer::OnUpdateLoginInfo(const UdpPacket* msg, const sockaddr* srcAddr) {

	OnAckClient(ACK_LOGIN, msg, srcAddr);
	ForwardTable table;
	table.addr = *reinterpret_cast<const sockaddr_in*>(srcAddr);
	table.sessionId = msg->sessionId;
	table.localUdpPort = msg->localPort;
	table.expireTime = uv_now(m_pLoop) + 3 * m_nTimeout;

	m_mapForwardTable.insert(make_pair(msg->userId, table));
	return 0;
}
int CMediaServer::OnLogout(int userId)
{
	auto forwardTable = m_mapForwardTable.find(userId);

	LOGFMTI("receive LogoutMsg,userId:%u\n", userId);
	if (forwardTable == m_mapForwardTable.end()) {
		LOGFMTI("can't find userId %u on logout", userId);
		return -1;
	}
	for (int delUserId : forwardTable->second.dataFrom) {
		if (userId == delUserId)
			continue;
		auto it = m_mapForwardTable.find(delUserId);
		if(it!=m_mapForwardTable.end())
			it->second.dataTo.erase(userId);
	}
	for (auto dataTo : forwardTable->second.dataTo) {
		if (userId == dataTo.first)
			continue;
		auto it = m_mapForwardTable.find(dataTo.first);
		if(it!=m_mapForwardTable.end())
			it->second.dataFrom.erase(userId);
	}
	m_mapForwardTable.erase(forwardTable);
	return 0;
}

void CMediaServer::AfterVerification(uv_work_t* req, int status) {
	VerifyCtx *ctx = (VerifyCtx*)req->data;
	switch (ctx->msg->type) {
	//case UPDATE_FORWARD_TABLE_MSG:
	//	if (ctx->success == 0)//success
	//		ctx->server->OnUpdateForwardInfo(ctx->msg, ctx->srcAddr);
	//	//else//ack
	//		//ctx->server->OnAckClient(ACK_UPDATE_FORWARDTABLE, ctx->msg, ctx->srcAddr);
	//	break;
	case LOGIN_MSG:
		if (ctx->success == 0)
			ctx->server->OnUpdateLoginInfo(ctx->msg, ctx->srcAddr);
		break;
	}
	FreeVerifyCtx(ctx);
	free(req);
}

void CMediaServer::OnAckClient(const int msg,const UdpPacket*packet,const sockaddr *srcAddr) {
	UdpPacket ack = *packet;
	ack.del = 0;
	ack.type = msg;
	switch (msg) {
	case ACK_P2P_ADDR:
		ack.del = 1;break;
	}
	this->send((char*)&ack, sizeof(ack), (sockaddr_in*)srcAddr);
}
int CMediaServer::OnRecvSingleForwardMsg(const UdpPacket& msg, const char* buf, int len, const sockaddr* srcAddr) {
	auto receiver = m_mapForwardTable.find(msg.dstUserId);
	if (receiver == m_mapForwardTable.end()) {
		LOGFMTI("can't find the forward target,userId:%u", msg.dstUserId);
		return -1;
	}
	this->send( (char*)buf, len, &receiver->second.addr);
	return 0;
}

int CMediaServer::OnRecvMultiForwardMsg(const UdpPacket& msg, const char* buf, int len, const sockaddr* srcAddr) {
	auto forwardTable = m_mapForwardTable.find(msg.userId);

	if (forwardTable == m_mapForwardTable.end()) {
		LOGFMTI("can't find the forward table of userId %u\n", msg.userId);
		return -1;
	}
	addr_verification(srcAddr, &forwardTable->second.addr);
#ifdef WIN32
	for (auto &dstClient: forwardTable->second.dataTo) {
		this->send(buf, len, &dstClient.second);
	}
#else
	struct msghdr msg2;
	struct iovec iov;

	msg2.msg_iov = &iov;
	msg2.msg_iovlen = 1;
	msg2.msg_iov->iov_base = (char*)buf;
	msg2.msg_iov->iov_len = len;
	msg2.msg_control = 0;
	msg2.msg_controllen = 0;
	msg2.msg_flags = 0;
	msg2.msg_namelen = sizeof(sockaddr_in);

	for (auto &dstClient : forwardTable->second.dataTo) {
		msg2.msg_name = &dstClient.second;
		sendmsg(this->m_udpHandle.io_watcher.fd, &msg2, 0);
	}
#endif
	return 0;
}
// Used for test. Now operation for forwardtable is transmit with TCP protocol
//int CMediaServer::OnRecvUpdateForwardTableMsg(const UdpPacket& msg, const char* buf, int len, const sockaddr* srcAddr) {
//	LOGFMTI("receive UpdateForwardTableMsg,userId:%u to userId:%u\n", msg.userId, msg.dstUserId);
//
//	uv_work_t* req = (uv_work_t*)malloc(sizeof(uv_work_t));
//	req->data = InitAndAllocVerifyCtx(this,(UdpPacket*)&msg,srcAddr);
//	uv_queue_work(m_pLoop, req, ID_Verification, AfterVerification);
//
//	return 0;
//}

//int CMediaServer::OnUpdateForwardInfo(const UdpPacket* msg, const sockaddr* srcAddr){
//	auto src = m_mapForwardTable.find(msg->userId);
//	auto dst = m_mapForwardTable.find(msg->dstUserId);
//
//	if (src == m_mapForwardTable.end()) {
//		LOGFMTI("can't find the forward table of userId %u\n", msg->userId);
//		return -1;
//	}
//	if (dst == m_mapForwardTable.end()) {
//		LOGFMTI("can't find the forward table of userId %u\n", msg->dstUserId);
//		return -1;
//	}
//	addr_verification(srcAddr, &src->second.addr);
//	OnAckClient(ACK_UPDATE_FORWARDTABLE, msg, srcAddr);
//	if (msg->del) {//add
//		src->second.dataTo[msg->dstUserId] = dst->second.addr;
//		dst->second.dataFrom.insert(msg->userId);
//	}
//	else {//del
//		src->second.dataTo.erase(msg->dstUserId);
//		dst->second.dataFrom.erase(msg->userId);
//	}
//	return 0;
//}

int CMediaServer::OnRecvLogoutMsg(const UdpPacket& msg, const char* buf, int len,const sockaddr* srcAddr) {
	OnAckClient(ACK_LOGOUT, &msg, srcAddr);

	auto forwardTable = m_mapForwardTable.find(msg.userId);

	//printf("receive LogoutMsg,userId:%u\n", msg.userId);
	if (forwardTable == m_mapForwardTable.end()) {
		LOGFMTI("can't find userId %u on logout",msg.userId);
		return -1;
	}
	addr_verification(srcAddr, &forwardTable->second.addr);
	for (int delUserId : forwardTable->second.dataFrom) {
		m_mapForwardTable[delUserId].dataTo.erase(msg.userId);
	}
	m_mapForwardTable.erase(msg.userId);
	return 0;
}

void CMediaServer::ID_Verification(uv_work_t*req) {
	VerifyCtx *ctx = (VerifyCtx*)req->data;
	switch (ctx->msg->type) {
	case LOGIN_MSG:
		ctx->success = ctx->server->m_strValidateUrlOnLogin!=""?
			ctx->server->m_pHttpClient->ValidateOnLogin(ctx->msg->userId,ctx->msg->sessionId,&ctx->server->m_strValidateUrlOnLogin)
			:0;
		break;
	default:
		ctx->success = ctx->server->m_strValidateUrl != "" ?
			ctx->server->m_pHttpClient->Validate(ctx->msg->userId, ctx->server->m_mapForwardTable[ctx->msg->userId].sessionId, ctx->msg->dstUserId, &ctx->server->m_strValidateUrl) :
			0;
	}
}

VerifyCtx* InitAndAllocVerifyCtx(CMediaServer *server, UdpPacket *msg, const sockaddr* srcAddr) {
	VerifyCtx *ctx = (VerifyCtx*)malloc(sizeof(VerifyCtx));
	ctx->msg = (UdpPacket*)malloc(sizeof(UdpPacket));
	*ctx->msg = *msg;
	ctx->srcAddr = (sockaddr*)malloc(sizeof(sockaddr));
	*ctx->srcAddr = *srcAddr;
	ctx->server = server;
	return ctx;
}
void FreeVerifyCtx(VerifyCtx* ctx) {
	free(ctx->msg);
	free(ctx->srcAddr);
	free(ctx);
}

void CMediaServer::OnUvThreadMessage(const CReqUpdateForwardTableMsg &msg, unsigned int nSrcAddr) {
	auto src = m_mapForwardTable.find(msg.srcUserId);
	auto dst = m_mapForwardTable.find(msg.dstUserId);
	if (src == m_mapForwardTable.end() || dst == m_mapForwardTable.end()) {
		LOGI("can't find forward table of user");
		return;
	}
	if (msg.add) {
		src->second.dataTo[msg.dstUserId] = dst->second.addr;
		dst->second.dataFrom.insert(msg.srcUserId);
	}
	else {
		src->second.dataTo.erase(msg.dstUserId);
		dst->second.dataFrom.erase(msg.srcUserId);
	}
}