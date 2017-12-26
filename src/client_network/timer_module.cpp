
#include "network_module.h"
#include "timer_module.h"
#include "udp_module.h"

const int heartBeatTime = 10000;
void CTimerManager::LoginTimer(uv_timer_t * timer)
{
	TimerCtx *ctx = (TimerCtx*)timer->data;
	if (ctx->timerManager->m_pUdpClient->m_bLoginSuccess) {
		uv_timer_set_repeat(timer, heartBeatTime);
	}
	ctx->timerManager->m_pUdpClient->send((char*)ctx->msg, sizeof(UdpPacket));
	if (--ctx->repeatTimes == 0) {
		PRINT_DEBUG("login timeout\n");
		ctx->timerManager->removeTimer(ctx->msg->index);
	}
}

void CTimerManager::P2PTimer(uv_timer_t * timer)
{
	TimerCtx *ctx = (TimerCtx*)timer->data;
	CUDPModule *udpClient = ctx->timerManager->m_pUdpClient;
	int dstUserId = ctx->msg->dstUserId;
	auto it = udpClient->m_mapPeerClientCtx.find(dstUserId);
	if (it == udpClient->m_mapPeerClientCtx.end()) {
		ctx->timerManager->removeP2PTimer(dstUserId);
		PRINT_DEBUG("P2PTimer error\n");
		return;
	}
	sockaddr_in* addr = ctx->msg->del == 0 ? &it->second.LANAddr : &it->second.WANAddr;
	udpClient->CUDPHandle::send((char*)ctx->msg, sizeof(UdpPacket),addr);
	if (ctx->repeatTimes-- == 0) {
		if (ctx->msg->del == 0) {
			PRINT_DEBUG("LAN P2P timeout\n");
			udpClient->tryWANP2PConnect(dstUserId);
		}
		else {
			PRINT_DEBUG("WAN P2P timeout\n");
			ctx->timerManager->removeP2PTimer(dstUserId);
			it->second.status = P2PStatus::requireRelay;
			ClientNetwork::AfterP2PFail(dstUserId);
		}
	}
}

CTimerManager::CTimerManager(CUDPModule *udpClient):
	m_pUdpClient(udpClient)
{

}

CTimerManager::~CTimerManager()
{
	
}

void CTimerManager::gernerateLoginTimer(UdpPacket *msg,int repeatTimes,int repeatDelay)
{
	char index;
	do {
		index = rand();
	} while (m_mapTimer.find(index) != m_mapTimer.end());
	uv_timer_t *timer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
	m_mapTimer[index] = timer;
	timer->data = generateTimerCtx(msg, repeatTimes,index);
	
	uv_timer_init(m_pUdpClient->m_pLoop, timer);
	uv_timer_start(timer, LoginTimer,0,repeatDelay);
}

void CTimerManager::gernerateP2PTimer(UdpPacket *msg,int repeatTimes,int repeatDelay)
{
	uv_timer_t *timer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
	m_mapP2PTimer[msg->dstUserId] = timer;

	timer->data = generateTimerCtx(msg, repeatTimes, 0);
	uv_timer_init(m_pUdpClient->m_pLoop, timer);
	uv_timer_start(timer, P2PTimer, 0, repeatDelay);
}

void CTimerManager::removeTimer(int index)
{
	auto it = m_mapTimer.find(index);
	if (it != m_mapTimer.end()) {
		uv_timer_stop(it->second);
		freeTimerCtx((TimerCtx*)it->second->data);
		uv_close(reinterpret_cast<uv_handle_t*>(it->second), [](uv_handle_t* handle) {free(handle); });
		m_mapTimer.erase(it);
	}
}

void CTimerManager::removeP2PTimer(int index)
{
	auto it = m_mapP2PTimer.find(index);
	if (it != m_mapP2PTimer.end()) {
		uv_timer_stop(it->second);
		freeTimerCtx((TimerCtx*)it->second->data);
		uv_close(reinterpret_cast<uv_handle_t*>(it->second), [](uv_handle_t* handle) {free(handle); });
		m_mapP2PTimer.erase(it);
	}
}

void CTimerManager::clearP2PTimer()
{
	while (m_mapP2PTimer.size() > 0)
		removeP2PTimer(m_mapP2PTimer.begin()->first);
}

void CTimerManager::clearTimer()
{
	while (m_mapTimer.size() > 0)
		removeTimer(m_mapTimer.begin()->first);
}

TimerCtx *CTimerManager::generateTimerCtx(UdpPacket *msg, int repeatTimes,int index)
{
	TimerCtx *ctx = (TimerCtx*)malloc(sizeof(TimerCtx));
	msg->index = index;
	ctx->msg = (UdpPacket*)malloc(sizeof(UdpPacket));
	*ctx->msg = *msg;
	ctx->repeatTimes = repeatTimes;
	ctx->timerManager = this;
	return ctx;
}

void CTimerManager::freeTimerCtx(TimerCtx * ctx)
{
	free(ctx->msg);
	free(ctx);
}
