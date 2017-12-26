#pragma once
#include "SimpleLocks.h"
#include "SimpleUVExport.h"
#include "UVMsgFraming.h"

//函数
#if defined (WIN32) || defined(_WIN32)
#define uv_thread_close(t) (CloseHandle(t)!=FALSE)
#define uv_thread_sleep(ms) Sleep(ms);//睡眠ms毫秒
#define uv_thread_id GetCurrentThreadId//得到当前线程句柄

//#elif defined(__linux__)
#else
#include <unistd.h>
#define uv_thread_close(t) ()
#define uv_thread_sleep(ms) usleep((ms) * 1000)
#define uv_thread_id pthread_self//得到当前线程句柄
/*
#else
#error "no supported os"
*/
#endif

#define BEGIN_UV_THREAD_BIND virtual void OnDispatchMsg(unsigned int nMsgType, void *data, unsigned int nSrcAddr) \
	{ \

#define UV_THREAD_BIND(MsgType, MSG_CLASS) \
	if (MsgType == nMsgType) \
		{ \
		MSG_CLASS *pMsg = (MSG_CLASS *)data; \
		this->OnUvThreadMessage(*pMsg, nSrcAddr); \
		delete pMsg; pMsg = nullptr; return ; \
		} \

#define END_UV_THREAD_BIND(BASE_CLASS) return BASE_CLASS::OnDispatchMsg(nMsgType, data, nSrcAddr); \
	} \

#define END_BASE_UV_THREAD_BIND return ; } \

class SUV_EXPORT CThreadMsgBase
{
public:
	CThreadMsgBase(void);
	virtual ~CThreadMsgBase(void);

	template<class TYPE>
	void PushBackMsg(unsigned int nMsgType, const TYPE &msg, unsigned int nSrcAddr = 0);

protected:
	virtual void PushBackMsg(NodeMsg *msg);  // 这个函数调用的时候注意传递的值
	void DispatchThreadMsg();

	BEGIN_UV_THREAD_BIND
	END_BASE_UV_THREAD_BIND

	CUVMutex m_lock;
	NodeMsg *m_pMsgTail;
	int m_nTcpHandle;
};


template<class TYPE>
void CThreadMsgBase::PushBackMsg(unsigned int nMsgType, const TYPE &msg, unsigned int nSrcAddr)
{
	NodeMsg *pMsg = new NodeMsg;
	pMsg->m_nMsgType = nMsgType;
	pMsg->m_nSrcAddr = nSrcAddr;
	TYPE *pData = new TYPE(msg);
	pMsg->m_pData = pData;

	this->PushBackMsg(pMsg);
}

