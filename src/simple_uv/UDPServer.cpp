#include "UDPServer.h"
// #include "log4z.h"

/*****************************************UDP Server*************************************************************/
CUDPServer::CUDPServer(uv_loop_t* pLoop):
	m_strServerIP("0.0.0.0"),
	m_nServerPort(12345),
	CUDPHandle(pLoop)
{
	ILog4zManager::getRef().start();
}

CUDPServer::~CUDPServer()
{

}

bool CUDPServer::Start(const char* ip, int port)
{
	if (ip) {
		m_strServerIP = ip;
		m_nServerPort = port;
	}
	if (!bind(m_strServerIP.c_str(), m_nServerPort)) {
		return false;
	}
	if (!setSocketBufSize(0x7fffffff, 0x7fffffff)) {
		LOGI("expand socket buf fail");
	}
	uv_udp_recv_start(&this->m_udpHandle, AllocBufferForUdpRecv, OnUdpRead);
	return true;
}

bool CUDPServer::Start6(const char* ip, int port)
{
	m_strServerIP = ip;
	m_nServerPort = port;
	if (!bind6(m_strServerIP.c_str(), m_nServerPort)) {
		return false;
	}
	uv_udp_recv_start(&this->m_udpHandle, AllocBufferForUdpRecv, OnUdpRead);
	return true;
}