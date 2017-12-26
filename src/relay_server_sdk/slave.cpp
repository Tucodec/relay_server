#include <simple_uv/log4z.h>
#include "slave.h"


CSlave::CSlave(uint32_t ip, uint16_t port):
	m_nSlaveServerIp(ip),
	m_nSlaveServerPort(port)
{

}


CSlave::~CSlave()
{
}

int CSlave::LoginAsSlave() {
	CSlaveLoginMsg msg;
	msg.addr = m_nSlaveServerIp;
	msg.port = m_nSlaveServerPort;
	return this->SendUvMessage(msg, msg.MSG_ID);
}

void CSlave::AfterReconnectSuccess()
{
	LoginAsSlave();
}

void CSlave::AfterDisconnected()
{

}
