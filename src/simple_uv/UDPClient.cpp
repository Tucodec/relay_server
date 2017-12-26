#include "UDPClient.h"
#include "log4z.h"

/*****************************************UDP Server*************************************************************/
CUDPClient::CUDPClient(uv_loop_t *pLoop) :
	CUDPHandle(pLoop)
{

}

CUDPClient::~CUDPClient()
{

}

bool CUDPClient::Connect(const char *ip,int port)
{
	int iret;
	iret = uv_ip4_addr(ip, port, &m_serverAddr);
	if (iret)
		return false;
	iret = uv_udp_recv_start(&this->m_udpHandle, AllocBufferForUdpRecv, OnUdpRead);
	if (iret)
		return false;
	return true;
}

bool CUDPClient::send(const char*msg, const unsigned int msg_len) {
	return CUDPHandle::send(msg, msg_len, &m_serverAddr);
}