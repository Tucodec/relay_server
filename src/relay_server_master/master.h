#ifndef MASTER_H
#define MASTER_H
#include <unordered_set>
#include <unordered_map>
#include <simple_uv/tcpserver.h>
#include <relay_server_sdk/client_tcpserver_msg.h>

class CTestGateWay :
	public CTCPServer
{
public:
	CTestGateWay();
	virtual ~CTestGateWay();
	void LoadConfig();
	bool Start();

protected:
	BEGIN_UV_BIND
		UV_BIND(CSlaveLoginMsg::MSG_ID, CSlaveLoginMsg)
		UV_BIND(CSlaveStatusReportMsg::MSG_ID, CSlaveStatusReportMsg)
		UV_BIND(CReqSlaveAddrMsg::MSG_ID, CReqSlaveAddrMsg)
	END_UV_BIND(CTCPServer)

	int OnUvMessage(const CSlaveLoginMsg &msg, TcpClientCtx *pClient);
	int OnUvMessage(const CSlaveStatusReportMsg &msg, TcpClientCtx *pClient);
	int OnUvMessage(const CReqSlaveAddrMsg &msg, TcpClientCtx *pClient);

	virtual void CloseCB(int clientid);

private:
	struct SlaveNode {
		uint32_t ip;
		uint16_t port;
	};
	//map<int, TcpClientCtx *> m_mapSession;
	std::vector<int>m_vectorIndex;  // store slaveAddr index ,used for random mod :[clientid,clientid...]
	std::unordered_map<int, SlaveNode>m_mapSlaveAddr;  // <index=clientid,ip:port>
	std::unordered_set<uint32_t>m_setSlaveNodeWhitelist;  // <addr>
	std::unordered_map<uint32_t, int>m_mapSessionIndex;  // <session,index of mapSlaveAddr>

	int AllocateServerToSessionId(int sessionId);

	std::string m_strMasterIp;
	int m_nMasterPort;
};
#endif