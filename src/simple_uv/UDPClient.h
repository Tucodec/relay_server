#pragma once
#include "UDPHandle.h"

class SUV_EXPORT CUDPClient : public CUDPHandle
{
public:
	CUDPClient(uv_loop_t* pLoop);
	virtual ~CUDPClient();
	bool  Connect(const char *ip, int port);//Start the client, ipv4

	template<class TYPE>
	int SendUvThreadMessage(const TYPE& msg, unsigned int nMsgType);
protected:
	sockaddr_in m_serverAddr;
	bool  send(const char*msg, const unsigned int msg_len);
};

template<class TYPE>
int CUDPClient::SendUvThreadMessage(const TYPE& msg, unsigned int nMsgType)
{
	this->PushBackMsg(msg.MSG_ID, msg);
	uv_async_send(&this->m_asyncHandleForRecvMsg);
	return 0;
}