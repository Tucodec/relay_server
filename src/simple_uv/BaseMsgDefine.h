#pragma once
#include "uv.h"
#include "PacketSync.h"


// class AcceptClient;
typedef struct _tcpclient_ctx {
	uv_tcp_t tcphandle;//data filed store this
	CPacketSync* packet_;//userdata filed storethis
	uv_buf_t read_buf_;
	int clientid;
	void* parent_server;//tcpserver
	void* parent_acceptclient;//accept client
} TcpClientCtx;
TcpClientCtx* AllocTcpClientCtx(void* parentserver);
void FreeTcpClientCtx(TcpClientCtx* ctx);

typedef struct _write_param { //the param of uv_write
	uv_write_t write_req_;
	uv_buf_t buf_;
	size_t buf_truelen_;
} write_param;
write_param* AllocWriteParam(void);
void FreeWriteParam(write_param* param);

#define BASE_MSG_BEGIN 10000

enum 
{
	REGIST_THREAD_MSG = BASE_MSG_BEGIN,
	UN_REGIST_THREAD_MSG
};

class CRegistMsg
{
public:
	enum {
		MSG_ID = REGIST_THREAD_MSG
	};

	~CRegistMsg()
	{
		m_pData = nullptr;
	}

	unsigned int m_nType;
	void *m_pData;
};