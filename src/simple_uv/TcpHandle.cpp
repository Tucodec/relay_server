// #include "stdafx.h"
#include "TcpHandle.h"
#include "UVThread.h"
#include "BaseMsgDefine.h"
#include <assert.h>
#include "UVMsgFraming.h"
#include "UVThreadMng.h"
#include "LogMng.h"

CTCPHandle::CTCPHandle()
	: m_bIsClosed(true)
	, m_bIsUserAskForClosed(false)
	, m_cPacketHead(SERVER_PACKET_HEAD)
	, m_cPacketTail(SERVER_PACKET_TAIL)
{
	m_nTcpHandle = 1;
	int iret = uv_loop_init(&m_loop);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		fprintf(stdout, "init loop error: %s\n", m_strErrMsg.c_str());
	}
	iret = uv_mutex_init(&m_mutexClients);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		fprintf(stdout, "uv_mutex_init error: %s\n", m_strErrMsg.c_str());
	}
}


CTCPHandle::~CTCPHandle(void)
{
	uv_mutex_destroy(&m_mutexClients);
}

void  CTCPHandle::Close()
{
	if (m_bIsClosed) {
		return;
	}
}

uv_loop_t *CTCPHandle::GetLoop() {
	return &m_loop;
}

bool  CTCPHandle::SetNoDelay(bool enable)
{
	int iret = uv_tcp_nodelay(&m_tcpHandle, enable ? 1 : 0);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		return false;
	}
	return true;
}

bool  CTCPHandle::SetKeepAlive(int enable, unsigned int delay)
{
	int iret = uv_tcp_keepalive(&m_tcpHandle, enable, delay);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		return false;
	}
	return true;
}

bool CTCPHandle::init()
{
	if (!m_bIsClosed) {
		return true;
	}
	int iret = uv_async_init(&m_loop, &m_asyncHandle, AsyncCloseCB);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		return false;
	}
	m_asyncHandle.data = this;

	uv_async_init(&m_loop, &m_asyncHandleForRecvMsg, AsyncRecvMsg);
	m_asyncHandleForRecvMsg.data = this;

	iret = uv_tcp_init(&m_loop, &m_tcpHandle);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		return false;
	}
	m_tcpHandle.data = this;
	
	m_bIsClosed = false;
	return true;
}

void CTCPHandle::AsyncCloseCB( uv_async_t* handle )
{
	CTCPHandle* theclass = (CTCPHandle*)handle->data;
	if (theclass->m_bIsUserAskForClosed) {
		theclass->closeinl();
		return;
	}
	//check data to send
	theclass->send_inl(NULL);
}

void  CTCPHandle::send_inl( uv_write_t* req /*= NULL*/ )
{

}

int CTCPHandle::ParsePacket(const NetPacket& packet, const unsigned char* buf, TcpClientCtx *pClient)
{
	return -1;
}

void CTCPHandle::closeinl()
{
	
}

bool CTCPHandle::run(int status /*= UV_RUN_DEFAULT*/)
{
	int iret = uv_run(&m_loop, (uv_run_mode)status);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		this->OnExit();
		return false;
	}
	this->OnExit();
	return true;
}

void CTCPHandle::AsyncRecvMsg( uv_async_t* handle )
{
	CTCPHandle* pClass = (CTCPHandle*)handle->data;

	pClass->DispatchThreadMsg();
}


void CTCPHandle::OnExit()
{

}


void GetPacket(const NetPacket& packethead, const unsigned char* packetdata, void* userdata)
{
	//fprintf(stdout, "Get control packet type %d\n", packethead.type);
	assert(userdata);
	TcpClientCtx* theclass = (TcpClientCtx*)userdata;
	CTCPHandle* parent = (CTCPHandle*)theclass->parent_server;
	parent->ParsePacket(packethead, packetdata, theclass);
	return;
}
