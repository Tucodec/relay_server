#include "UVThread.h"
#include "UVThreadMng.h"
#include "UVMsgFraming.h"



void CUVThread::ThreadFun(void* arg)
{
	CUVThread *pThread = (CUVThread *)arg;

	if (!pThread->OnInit())
	{
		pThread->Run();
	}	

	uv_thread_join(&pThread->m_thread);
}

void CUVThread::Run()
{
	this->DispatchThreadMsg();
}

void CUVThread::OnUvThreadMessage(CRegistMsg msg, unsigned int nSrcAddr)
{
	m_mapThread->insert(map<unsigned int, CUVThread*>::value_type(msg.m_nType, (CUVThread *)(msg.m_pData)));
}

void CUVThread::OnUvThreadMessage( unsigned int msg, unsigned int nSrcAddr )
{
	map<unsigned int, CUVThread *>::iterator it = m_mapThread->find(msg);

	if (it != m_mapThread->end())
	{
		m_mapThread->erase(it);
	}
}
int CUVThread::OnInit()
{
	CUVThreadMng::GetInstance()->RegistThread(m_nThreadType, this);

	return 0;
}

void CUVThread::OnExit()
{
	CUVThreadMng::GetInstance()->UnRegistThread(m_nThreadType);

	for (map<unsigned int, CUVThread *>::iterator it = m_mapThread->begin();
		it != m_mapThread->end(); )
	{
		m_mapThread->erase(it++);
	}

}

void CUVThread::Start()
{
	if (m_bIsRunning) {
		return;
	}
	uv_thread_create(&m_thread, ThreadFun, this);
	m_bIsRunning = true;
}

