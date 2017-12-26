
#include "ThreadMsgBase.h"


CThreadMsgBase::CThreadMsgBase(void)
	: m_pMsgTail(nullptr)
	, m_nTcpHandle(0)
{
}


CThreadMsgBase::~CThreadMsgBase(void)
{
}

void CThreadMsgBase::PushBackMsg(NodeMsg *msg)
{
	m_lock.Lock();
	if (!m_pMsgTail)
	{
		msg->next = msg;
		m_pMsgTail = msg;
	}
	else
	{
		msg->next = m_pMsgTail->next;
		m_pMsgTail->next = msg;
		m_pMsgTail = msg;
	}
	m_lock.UnLock();
}

void CThreadMsgBase::DispatchThreadMsg()
{
	NodeMsg* req;
	NodeMsg* first;
	NodeMsg* next;

	while (true)
	{
		if (m_pMsgTail == nullptr)
		{
			if (m_nTcpHandle)
			{
				break;
			}

			uv_thread_sleep(100);
			continue;
		}

		m_lock.Lock();
		first = m_pMsgTail->next;
		next = first;
		m_pMsgTail = nullptr;
		m_lock.UnLock();

		while (next != nullptr) {
			req = next;
			next = req->next != first ? req->next : nullptr;

			this->OnDispatchMsg(req->m_nMsgType, req->m_pData, req->m_nSrcAddr);

			delete req;
			req = nullptr;
		}

		if (m_nTcpHandle)
		{
			break;
		}
	}
}
