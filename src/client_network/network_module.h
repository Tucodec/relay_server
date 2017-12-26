#ifndef NETWORK_MODULE_H
#define NETWORK_MODULE_H
#include <stdint.h>
#include <memory>
#include "voip/network_center.h"
#include "network_module_export.h"

class CUDPModule;
class CTCPModule;
class CRspP2PAddrMsg;

typedef void(*NetworkChangeCB)(int status);
enum NetworkStatus{
	DISCONNECTED,
	CONNECTED
};

class CLIENT_NETWORK_EXPORT ClientNetwork: public NetworkCenter {
	std::shared_ptr<CUDPModule> m_pUdpClient;
	std::shared_ptr<CTCPModule> m_pTcpClient;
	int m_nSessionId;
	int m_nUserId;
	bool m_bInRoom;
	bool m_bEnableP2P;
	NetworkChangeCB m_pNetworkChangeCB;

	static ClientNetwork* s_pInstance;

	static void TryP2PConnect(uint32_t targetUserId);
	static void AfterGetP2PAddr(const CRspP2PAddrMsg &msg);
	static void AfterP2PFail(uint32_t targetUserId);

	friend class CUDPModule;
	friend class CTCPModule;
	friend class CTimerManager;

	VoipDataListener *m_pVoipDataListener;
	virtual int RegisterVoipDataListener(VoipDataListener *voipDataListener);
	virtual int UnRegisterVoipDataListener(VoipDataListener *voipDataListener);

    ClientNetwork(Parameter param);
public:
	~ClientNetwork();
	int Login(uint32_t userID, uint32_t sessionID);
	void AddGroupClientNode(uint32_t targetUserId);
	void RemoveGroupClientNode(uint32_t targetUserId);
	void ClearGroupClientList();
	void Logout();
	void setNetworkChangeCB(NetworkChangeCB pFun);
	void setP2PTransmit(bool enable);
	virtual int sendVoipData(char *data, int size);
    static ClientNetwork* CreateInstance(Parameter param);
	static ClientNetwork* GetInstance();
	static void DestroyInstance();
};

#ifdef NETWORK_DEBUG
	#define PRINT_DEBUG(fmt, ...) printf((fmt "[function:%s]" "[line:%d]\n" ),##__VA_ARGS__, __FUNCTION__, __LINE__);
#else
	#define PRINT_DEBUG(format, ...)
#endif  // DEBUG

#endif  // NETWORK_MODULE_H