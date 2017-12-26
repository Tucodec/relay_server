
#include <simple_uv/common.h>
#include <simple_uv/SimpleLocks.h>
#include "network_module.h"
#include "tcp_module.h"
#include "udp_module.h"

CUVMutex sMutex;
ClientNetwork* ClientNetwork::s_pInstance = nullptr;

ClientNetwork::ClientNetwork(Parameter param)
    : NetworkCenter(param)
    , m_pNetworkChangeCB(nullptr)
    , m_pVoipDataListener(nullptr)
    , m_bInRoom(false)
{

}

ClientNetwork::~ClientNetwork()
{
	Logout();
}

int ClientNetwork::Login(uint32_t userID, uint32_t sessionID)
{
	Logout();
	//connect to master server 
    const char * IP = m_param.serverIP.c_str();
    uint32_t port = m_param.port;
	m_nUserId = userID;
	bool masterServer = 0;
	std::string realIP;
	int realPort;
	if(masterServer) {
		CTCPModule tcpClient(0);
		if (!tcpClient.Connect(IP, port)) {
			PRINT_DEBUG("connect error:%s\n", tcpClient.GetLastErrMsg());
		}
		if (tcpClient.requestRelayServerAddr(m_nSessionId) != 0) {
			PRINT_DEBUG("timeout\n");
			return false;
		}
		realIP = ip_int2string(tcpClient.m_nRelayServerAddr);
		realPort = port_ntohs(tcpClient.m_nRelayServerPort);
	}
	else {
		realIP = string(IP);
		realPort = port;
	}
	m_pTcpClient = std::make_shared<CTCPModule>(userID);
	m_pUdpClient = std::make_shared<CUDPModule>(userID, sessionID,m_pTcpClient->GetLoop());
	// connect on tcp server
	if (!m_pTcpClient->Connect(realIP.c_str(), realPort)) {
		PRINT_DEBUG("connect error:%s\n", m_pTcpClient->GetLastErrMsg());
		Logout();
		return false;
	}
	m_pTcpClient->SetKeepAlive(1, 30);
	m_pTcpClient->bindUserIdOnServer();
	// connect on udp server
	if (!m_pUdpClient->Connect(realIP.c_str(),realPort)) {
		PRINT_DEBUG("socket setup fail\n");
		Logout();
		return false;
	}
	if (!m_pUdpClient->LoginRelayServer()) {
		PRINT_DEBUG("login fail\n");
		Logout();
		return false;
	}
	PRINT_DEBUG("login success\n");
	return true;
}

void ClientNetwork::AddGroupClientNode(uint32_t targetUserId)
{
	if (m_pTcpClient.get() == nullptr||m_pUdpClient.get() == nullptr )
		return;
	m_bInRoom = true;
	m_pUdpClient->AddTransmitNode(targetUserId);  // async notify thread
}

void ClientNetwork::RemoveGroupClientNode(uint32_t targetUserId)
{
	if (m_pTcpClient.get() == nullptr || m_pUdpClient.get() == nullptr)
		return ;
	m_pTcpClient->RemoveForwardNode(targetUserId);  // notify server
	CReqRemoveForwardNodeMsg msg;
	msg.targetUserId = targetUserId;
	m_pUdpClient->SendUvThreadMessage(msg,0);  // async notify thread
}

void ClientNetwork::ClearGroupClientList()
{
	if (m_pTcpClient.get() == nullptr || m_pUdpClient.get() == nullptr)
		return;
	CReqLeaveRoomMsg msg;
	msg.userId = m_nUserId;
	m_pTcpClient->SendUvMessage(msg, msg.MSG_ID);
	m_pUdpClient->SendUvThreadMessage(msg,msg.MSG_ID);
	int try_time = 20;
	while (try_time-- && m_bInRoom)  // no matter what, this function should return
		uv_thread_sleep(50);
}

void ClientNetwork::Logout()
{
	m_pTcpClient.reset();  // deconstruction
	m_pUdpClient.reset();
}

void ClientNetwork::setNetworkChangeCB(NetworkChangeCB pFun)
{
	 m_pNetworkChangeCB=pFun;
}

void ClientNetwork::setP2PTransmit(bool enable)
{
	if(m_pUdpClient.get())
		m_pUdpClient->m_bEnableP2P = enable;
}

int ClientNetwork::sendVoipData(char* data, int size)
{
	if (m_pUdpClient.get() == nullptr)
		return -1;
    char*base= (char*)malloc(size+4);
	memcpy(base, &m_nUserId, 4);
	memcpy(base+4, data, size);
	m_pUdpClient->SendMedia(base,size+4);
	return 0;
}
ClientNetwork* ClientNetwork::CreateInstance(Parameter param)
{
	if (s_pInstance == nullptr) {
		CUVAutoLock lock(&sMutex);
		if (s_pInstance == nullptr)
			s_pInstance = new ClientNetwork(param);
	}
	return s_pInstance;
}

ClientNetwork * ClientNetwork::GetInstance()
{
	return s_pInstance;
}

void ClientNetwork::DestroyInstance()
{
	delete ClientNetwork::s_pInstance;
	ClientNetwork::s_pInstance = nullptr;
}

void ClientNetwork::TryP2PConnect(uint32_t targetUserId)
{
	ClientNetwork::GetInstance()->m_pTcpClient->GetP2PAddr(targetUserId);
}

void ClientNetwork::AfterGetP2PAddr(const CRspP2PAddrMsg &msg)
{
	ClientNetwork::GetInstance()->m_pUdpClient->OnUvThreadMessage(msg,0);
}

void ClientNetwork::AfterP2PFail(uint32_t targetUserId)
{
	ClientNetwork::GetInstance()->m_pTcpClient->AddForwardNode(targetUserId);
}

int ClientNetwork::RegisterVoipDataListener(VoipDataListener * voipDataListener)
{
	m_pVoipDataListener = voipDataListener;
	return 0;
}

int ClientNetwork::UnRegisterVoipDataListener(VoipDataListener * voipDataListener)
{
	m_pVoipDataListener = nullptr;
	return 0;
}
