#ifndef TIMER_MODULE_H
#define TIMER_MODULE_H
#include <uv.h>
#include <unordered_map>
#include <simple_uv/UdpPacket.h>

class CTimerManager;
typedef struct timerCtx {
	CTimerManager *timerManager;
	UdpPacket *msg;
	int repeatTimes;
}TimerCtx;
class CUDPModule;
class CTimerManager {
	CUDPModule *m_pUdpClient;
	std::unordered_map<char, uv_timer_t*>m_mapTimer;
	std::unordered_map<int, uv_timer_t*>m_mapP2PTimer;
	static void LoginTimer(uv_timer_t *timer);
	static void P2PTimer(uv_timer_t *timer);
	TimerCtx*generateTimerCtx(UdpPacket*msg, int repeatTimes, int index);
	void freeTimerCtx(TimerCtx* ctx);
public:
	friend class CUDPModule;
	CTimerManager(CUDPModule *udpClient);
	~CTimerManager();
	void gernerateLoginTimer(UdpPacket *msg,int repeatTimes,int repeatDelay);
	void gernerateP2PTimer(UdpPacket *msg,int repeatTimes,int repeatDelay);
	void removeTimer(int index);
	void removeP2PTimer(int index);
	void clearP2PTimer();
	void clearTimer();
};
#endif  // TIMER_MODULE_H