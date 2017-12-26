#pragma once
#include "UDPHandle.h"

class SUV_EXPORT CUDPServer : public CUDPHandle
{
public:
	CUDPServer(uv_loop_t* pLoop);
	virtual ~CUDPServer();

	bool  Start(const char* ip=NULL, int port=12345);//Start the server, ipv4

	int m_nServerPort;
	std::string m_strServerIP;
protected:
	bool  Start6(const char* ip, int port);//Start the server, ipv6
};