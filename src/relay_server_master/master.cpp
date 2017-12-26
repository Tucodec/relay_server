#include "master.h"
#include <algorithm>
#include <simple_uv/config.h>
#include <simple_uv/log4z.h>
#include <simple_uv/common.h>

CTestGateWay::CTestGateWay():
	m_strMasterIp("0.0.0.0")
{
}


CTestGateWay::~CTestGateWay()
{
}

bool CTestGateWay::Start()
{
	LOGFMTI("server start at %s:%u", m_strMasterIp.c_str(), m_nMasterPort);
	return CTCPServer::Start(m_strMasterIp.c_str(), m_nMasterPort);
}

void CTestGateWay::LoadConfig() {
	const string file = "master.config";
	Config configure;
	if (!configure.FileExist(file)) {
		return;
	}
	configure.ReadFile(file);
	{
		string list;
		list = configure.Read("slave_node", list);
		stringstream ss(list);
		string ip;
		while (!ss.eof()) {
			getline(ss, ip, ';');
			m_setSlaveNodeWhitelist.insert(ip_string2int(ip));
		}
	}
	{
		//m_strMasterIp = "0.0.0.0";
		//m_strMasterIp = configure.Read("master_ip", m_strMasterIp);
		m_nMasterPort = configure.Read("master_port", 10000);
	}
}

int CTestGateWay::AllocateServerToSessionId(int sessionId)
{
	// index = 0 - onlineServerNumber
	int index = m_vectorIndex[rand() % m_vectorIndex.size()];
	m_mapSessionIndex[sessionId] = index;
	return index;
}

int CTestGateWay::OnUvMessage(const CSlaveLoginMsg &msg, TcpClientCtx *pClient)
{
	sockaddr_storage addr;
	int len = sizeof(addr);
	uv_tcp_getsockname(&pClient->tcphandle, reinterpret_cast<sockaddr*>(&addr), &len);
	char *ip_get = inet_ntoa(((sockaddr_in*)&addr)->sin_addr);
	int	ip = *(int*)&(((sockaddr_in*)&addr)->sin_addr);
	LOGFMTI("login as slave %s:%u\n", ip_get,msg.port);

	auto it = m_setSlaveNodeWhitelist.find(ip);
	if (it == m_setSlaveNodeWhitelist.end()) {
		LOGFMTE("unknown addr: %s:%u try to login", ip_get, msg.port);
		return -1;
	}
	if (m_mapSlaveAddr.find(pClient->clientid) != m_mapSlaveAddr.end()) {
		LOGFMTE("clientid dulplicate on slave login");
		return -1;
	}
	m_mapSlaveAddr[pClient->clientid] = SlaveNode{ msg.addr,msg.port };
	m_vectorIndex.push_back(pClient->clientid);
	return 0;
}
int CTestGateWay::OnUvMessage(const CSlaveStatusReportMsg &msg, TcpClientCtx *pClient)
{
	sockaddr_storage addr;
	int len = sizeof(addr);
	uv_tcp_getsockname(&pClient->tcphandle, (sockaddr*)&addr, &len);
	char *ip_str = inet_ntoa(((sockaddr_in*)&addr)->sin_addr);
	LOGFMTI("%d:%s:%u recv&&transmit %lld bytes",0,ip_str,0,msg.network_status);
	return 0;
}

int CTestGateWay::OnUvMessage(const CReqSlaveAddrMsg &msg, TcpClientCtx *pClient) {
	if (m_vectorIndex.size() == 0) {
		LOGE("number of online slave server is 0");
		return 0;
	}
	auto it = m_mapSessionIndex.find(msg.sessionId);
	std::unordered_map<int, SlaveNode>::iterator result;
	if (it == m_mapSessionIndex.end()) {  // It is the first time this sessionId join server
		int nSlaveServerIndex = AllocateServerToSessionId(msg.sessionId);
		result = m_mapSlaveAddr.find(nSlaveServerIndex);
	}
	else {  // there are server hashed for sessionId
		int nSlaveServerIndex = it->second;
		result = m_mapSlaveAddr.find(nSlaveServerIndex);
		if (result == m_mapSlaveAddr.end()) {   // but the hashed server is not online 
			LOGFMTE("server offline ! need to reallocate server");
			nSlaveServerIndex = AllocateServerToSessionId(msg.sessionId);
			result = m_mapSlaveAddr.find(nSlaveServerIndex);
		}
	}

	CRspSlaveAddrMsg reply;
	reply.addr = result->second.ip;
	reply.port = result->second.port;
	return this->SendUvMessage(reply,reply.MSG_ID,pClient);
}

void CTestGateWay::CloseCB(int clientid)
{
	auto it = m_mapSlaveAddr.find(clientid);
	if (it == m_mapSlaveAddr.end())
		return;
	LOGFMTE(" server disconnected!");
	m_mapSlaveAddr.erase(it);
}
