#pragma once
#include "ThreadMsgBase.h"
#include <uv.h>
#if defined (WIN32) || defined(_WIN32)
#define uv_thread_close(t) (CloseHandle(t)!=FALSE)
#define uv_thread_sleep(ms) Sleep(ms);//睡眠ms毫秒
#define uv_thread_id GetCurrentThreadId//得到当前线程句柄

#else// defined(__linux__)
#include <unistd.h>
#include <string.h>
#define uv_thread_close(t) ()
#define uv_thread_sleep(ms) usleep((ms) * 1000)
#define uv_thread_id pthread_self//得到当前线程句柄

//#else
//#error "no supported os"
#endif

#include <iostream>
#include <stdlib.h>
#include "UdpPacket.h"
#include "log4z.h"  
#include <memory>
using namespace zsummer::log4z;

class SUV_EXPORT CUDPHandle : public CThreadMsgBase
{
public:
	CUDPHandle(uv_loop_t *pLoop);
	virtual ~CUDPHandle(void);
	virtual void  Close();
	static void OnUdpRead(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags);
	static void AllocBufferForUdpRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void OnHandleClose(uv_handle_t* handle);
	static void AfterUdpSend(uv_udp_send_t* req, int status);
	static void AfterHandleClose(uv_handle_t* handle) {};
	uv_async_t m_asyncHandleForRecvMsg;
protected:
	virtual int  ParsePacket(const UdpPacket& packet, const char* buf, int len,const sockaddr* srcAddr) = 0;
	bool bind(const char* ip, int port);
	bool bind6(const char* ip, int port);
	bool setBroadcast(bool enable);
	bool setSocketBufSize(int recvBufSize, int sendBufSize);
	bool send(const char*msg,const unsigned int msg_len,const sockaddr_in *send_addr);
	uv_udp_t m_udpHandle;
	uv_loop_t* m_pLoop;
	uint64_t m_nSendByteCount;
	uint64_t m_nRecvByteCount;
private:
	static void AsyncRecvMsg(uv_async_t* handle);
};
