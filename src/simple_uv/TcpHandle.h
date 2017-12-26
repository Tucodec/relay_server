#ifndef TCP_HANDLE_H_23432358247985439
#define TCP_HANDLE_H_23432358247985439

#include <string>
#include "SimpleUVExport.h"
#include "uv.h"
#include "BaseMsgDefine.h"
#include "UVMsgFraming.h"
#include "ThreadMsgBase.h"

using namespace std;


#define SERVER_PACKET_HEAD 0x01
#define SERVER_PACKET_TAIL 0x02

#define BEGIN_UV_BIND virtual int  ParsePacket(const NetPacket& packet, const unsigned char* buf, TcpClientCtx *pClient) \
	{ switch(packet.type){

#define UV_BIND(MSG_TYPE, MSG_CLASS) \
			case MSG_TYPE :{\
				MSG_CLASS *msg = (MSG_CLASS *)buf; \
				return this->OnUvMessage(*msg, pClient); \
				} \

#define END_UV_BIND(BASE_CLASS) \
			default:{\
			return BASE_CLASS::ParsePacket(packet, buf, pClient);} \
		}\
	} \


class SUV_EXPORT CTCPHandle : public CThreadMsgBase
{
public:
	CTCPHandle();
	~CTCPHandle(void);

	virtual void  Close();    //send close command. verify IsClosed for real closed
	virtual int  ParsePacket(const NetPacket& packet, const unsigned char* buf, TcpClientCtx *pClient);
	bool  IsClosed() {        //verify if real closed
		return m_bIsClosed;
	};

	//Enable or disable Nagle¡¯s algorithm. must call after Server succeed start.
	bool  SetNoDelay(bool enable);

	//Enable or disable KeepAlive. must call after Server succeed start.
	//delay is the initial delay in seconds, ignored when enable is zero
	bool  SetKeepAlive(int enable, unsigned int delay);

	const  char* GetLastErrMsg() const {
		return m_strErrMsg.c_str();
	};
	uv_loop_t *GetLoop();
	uv_async_t m_asyncHandleForRecvMsg;
protected:
	BEGIN_UV_THREAD_BIND
	END_UV_THREAD_BIND(CThreadMsgBase)
	virtual bool init();
	virtual void OnExit();
	virtual void  send_inl(uv_write_t* req = NULL);  //real send data fun
	virtual void closeinl();  //real close fun
	bool run(int status = UV_RUN_DEFAULT);

	template<class TYPE>
	string PacketData(const TYPE& msg, size_t nMsgType);

	bool m_bIsClosed;
	bool m_bIsUserAskForClosed;
	char m_cPacketHead;  //protocol head
	char m_cPacketTail;  //protocol tail
	uv_async_t m_asyncHandle;
	uv_tcp_t m_tcpHandle;
	uv_loop_t m_loop;
	std::string m_strErrMsg;
	uv_mutex_t m_mutexClients;  //clients map mutex

	enum {
		CONNECT_TIMEOUT,
		CONNECT_FINISH,
		CONNECT_ERROR,
		CONNECT_DIS
	};

private:
	static void AsyncCloseCB(uv_async_t* handle);  //async close
	static void AsyncRecvMsg(uv_async_t* handle);  //async close	
};

template<class TYPE>
string CTCPHandle::PacketData(const TYPE& msg, size_t nMsgType)
{
	NetPacket tmppack;
	tmppack.type = nMsgType;
	tmppack.header = SERVER_PACKET_HEAD;
	tmppack.tail = SERVER_PACKET_TAIL;
	tmppack.datalen = sizeof(TYPE);
	return PacketData2(tmppack, (const unsigned char*)&msg);
}

void GetPacket(const NetPacket& packethead, const unsigned char* packetdata, void* userdata);

#endif
