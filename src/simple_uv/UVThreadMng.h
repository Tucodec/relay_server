#ifndef UV_THREAD_MNG_H_34525634543
#define UV_THREAD_MNG_H_34525634543

#include <map>
#include "UVThread.h"
#include "SimpleUVExport.h"
#include "TcpHandle.h"
using namespace std;

class CTCPHandle;

class  CUVThreadMng
{
public:
	
	static SUV_EXPORT CUVThreadMng* GetInstance();
	void RegistThread(unsigned int nType, CUVThread *pThread);
	void UnRegistThread(unsigned int nType);
	void RegistHandle(uv_async_t *handle, CThreadMsgBase *pHandle);
	//void RegistHandle(uv_async_t *handle, CUDPHandle *pHandle);
	void UnRegistHandle();

	template<class TYPE>
	int  SendUvMessage(const TYPE& msg, size_t nMsgType, unsigned int nDstAddr, unsigned int nSrcAddr = 0)
	{
		m_lock.ReadLock();
		map<unsigned int, CUVThread *>::iterator it = m_mapThread.find(nDstAddr);

		if (it == m_mapThread.end())
		{
			m_lock.ReadUnLock();
			return -1;
		}

		it->second->PushBackMsg(nMsgType, msg, nSrcAddr);
		m_lock.ReadUnLock();

		return 0;
	}

	template<class TYPE>
	int  SendUvMessage2Handle(const TYPE& msg, size_t nMsgType, unsigned int nSrcAddr = 0)
	{
		m_pHandle->PushBackMsg(nMsgType, msg, nSrcAddr);

		uv_async_send(m_pHandleAsync);

		return 0;
	}

private:
	CUVThreadMng();
	~CUVThreadMng();
	uv_async_t *m_pHandleAsync;
	CThreadMsgBase *m_pHandle;
	map<unsigned int, CUVThread *> m_mapThread;
	CUVRWLock            m_lock;
	static CUVThreadMng* m_pMng;
	static CUVMutex      m_Mutex;
};


#endif

