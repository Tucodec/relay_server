#ifndef CLIENT_THREAD_MSG_H
#define CLIENT_THREAD_MSG_H
#pragma pack(1)
enum ClientThreadMsg{
	SEND_MEDIA_MSG = 0,
	ADD_FORWARD_NODE_MSG = 1,
	REMOVE_FORWARD_NODE_MSG = 2,
	REQ_UDP_LOGIN_MSG = 3
	//LEAVE_ROOM_MSG,
};
class CReqSendMediaMsg {
public:
	enum {
		MSG_ID= ClientThreadMsg::SEND_MEDIA_MSG
	};
	char*base;  // receiver free
	int len;
};
class CReqRemoveForwardNodeMsg {
public:
	enum {
		MSG_ID= ClientThreadMsg::REMOVE_FORWARD_NODE_MSG
	};
	int targetUserId;
};
class CReqUdpLoginMsg {
public:
	enum {
		MSG_ID= ClientThreadMsg::REQ_UDP_LOGIN_MSG
	};
	char reserve;
};
class CReqAddForwardNodeMsg {
public:
	enum {
		MSG_ID= ClientThreadMsg::ADD_FORWARD_NODE_MSG
	};
	int targetUserId;
};
#pragma pack()
#endif  // CLIENT_THREAD_MSG_H