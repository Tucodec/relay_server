/***************************************
* @file     tcpclient.h
****************************************/
#ifndef TCPCLIENT_H
#define TCPCLIENT_H
#include <string>
#include <list>
#include "uv.h"
#include "PacketSync.h"
#include "PodCircularBuffer.h"
#include "BaseMsgDefine.h"
#include "SimpleUVExport.h"
#include "TcpHandle.h"
#ifndef BUFFER_SIZE
#define BUFFER_SIZE (1024*10)
#endif


class SUV_EXPORT CTCPClient : public CTCPHandle
{
public:
	 CTCPClient();
	virtual  ~CTCPClient();
	static  void StartLog(const char* logpath = nullptr);
	static  void StopLog();
public:
	bool  Connect(const char* ip, int port);        //connect the server, ipv4
	bool  Connect6(const char* ip, int port);       //connect the server, ipv6
	int   Send(const char* data, std::size_t len);  //send data to server
	void  Close();                                  //send close command. verify IsClosed for real closed
	virtual int  ParsePacket(const NetPacket& packet, const unsigned char* buf, TcpClientCtx *pClient);

	template<class TYPE>
	int SendUvMessage(const TYPE& msg, size_t nMsgType);

protected:
	virtual bool  init();
	virtual void recvdata(const unsigned char* data, size_t len);
	void  closeinl();                                                        //real close fun
	void ReConnectCB(NET_EVENT_TYPE eventtype = NET_EVENT_TYPE_RECONNECT);
	virtual void AfterReconnectSuccess()  {};
	virtual void AfterDisconnected() {};
	void  send_inl(uv_write_t* req = NULL);                                  //real send data fun
	void CloseCB(int clientid, void* userdata);

	static void ConnectThread(void* arg);                                    //connect thread,run until use close the client
	static void AfterConnect(uv_connect_t* handle, int status);
	static void AfterRecv(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
	static void AfterSend(uv_write_t* req, int status);
	static void AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void AfterClientClose(uv_handle_t* handle);
	static void CloseWalkCB(uv_handle_t* handle, void* arg);                  //close all handle in loop
	static void ReconnectTimer(uv_timer_t* handle);

private:
	bool StartReconnect(void);
	void StopReconnect(void);

	bool m_bIsIPv6;
	bool m_bIsReconnecting;
	bool m_bTcpHandleClosed;
	
	int m_nServerPort;
	int m_nConnectStatus;
	int64_t m_nRepeatTime;    //repeat reconnect time. y=2x(x=1..)

	std::string m_strServerIP;

	uv_thread_t m_threadConnect;
	uv_connect_t m_connetcReq;
	uv_timer_t m_timerReconnet;

	TcpClientCtx *m_handleClient;

	std::list<write_param*> m_listWriteParam;    //Availa write_t
	CPodCircularBuffer<char> m_buferWrite;         //the data prepare to send
};

#endif // TCPCLIENT_H

template<class TYPE>
int CTCPClient::SendUvMessage(const TYPE& msg, size_t nMsgType)
{
	string str = this->PacketData(msg, nMsgType);
	return this->Send(&str[0], str.length());
}
