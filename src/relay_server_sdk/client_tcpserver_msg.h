#ifndef TEST_MESSAGE_H_4358943
#define TEST_MESSAGE_H_4358943
#include <stdint.h>
#pragma pack(1)

enum 
{
	TEST_MSG = 11000,
	SLAVE_LOGIN_MSG = 11001,
	SLAVE_STATUS_REPORT_MSG = 11002,
	GET_SLAVE_ADDR_MSG = 11003,
	REQ_UPDATE_FORWARDTABLE = 11004,
	REQ_P2PADDR = 11005,
	RSP_P2PADDR = 11006,
	REQ_LOGOUT = 11007,
	REQ_LOGIN = 11008,
	REQ_BIND_USERID = 11009,
	REQ_LEAVE_ROOM_MSG = 11010
};

class CSlaveLoginMsg {
public:
	enum {
		MSG_ID=SLAVE_LOGIN_MSG
	};
	uint32_t addr;
	uint16_t port;
};

class CSlaveStatusReportMsg {
public:
	enum {
		MSG_ID=SLAVE_STATUS_REPORT_MSG
	};
	uint64_t network_status;
	uint32_t client_number;
};
class CReqSlaveAddrMsg {
public:
	enum {
		MSG_ID = GET_SLAVE_ADDR_MSG
	};
	uint32_t sessionId;
};
class CRspSlaveAddrMsg {
public:
	enum {
		MSG_ID = GET_SLAVE_ADDR_MSG
	};
	uint32_t addr;
	uint16_t port;
};
class CReqUpdateForwardTableMsg {
public:
	enum {
		MSG_ID=REQ_UPDATE_FORWARDTABLE
	};
	uint32_t srcUserId;
	uint32_t dstUserId;
	bool add;
};
class CReqP2PAddrMsg {
public:
	enum {
		MSG_ID=REQ_P2PADDR
	};
	uint32_t srcUserId;
	uint32_t dstUserId;
	int clientId;
};
class CRspP2PAddrMsg {
public:
	enum {
		MSG_ID=RSP_P2PADDR
	};
	uint32_t dstUserId;
	uint32_t dstIp;
	uint16_t dstPort;
	uint16_t dstLocalUdpPort;
	int clientId;
	bool success;
};
class CReqLogoutMsg {
public:
	enum {
		MSG_ID=REQ_LOGOUT
	};
	uint32_t userId;
};
class CReqBindUserId {
public:
	enum {
		MSG_ID =REQ_BIND_USERID
	};
	uint32_t userId;
};
class CReqLeaveRoomMsg {
public:
	enum {
		MSG_ID = REQ_LEAVE_ROOM_MSG
	};
	int userId;
};
#pragma pack()
#endif