#ifndef RELAY_SERVER_H
#define RELAY_SERVER_H
#include <stdint.h>
#include <memory>
#include <string>
class CTurnServer;
class CSignalingServer;
class CRelayServer {
	std::shared_ptr<CSignalingServer>m_pTcpServer;
	std::shared_ptr<CTurnServer>m_pUdpServer;
	int m_nServerPort;
	int m_nMasterPort;
	std::string m_strServerIP;
	std::string m_strMasterIP;
	std::string m_strValidateUrl;
	std::string m_strValidateUrlOnLogin;
public:
	CRelayServer();
	~CRelayServer();
	bool Start();
	virtual int Validate(uint32_t uid1, uint32_t sessionId, uint32_t uid2, std::string *url);
	virtual int ValidateOnLogin(uint32_t uid1, uint32_t sessionId, std::string *url);
	void Close();
	void LoadConfiguration();
};
#endif  // RELAY_SERVER_H