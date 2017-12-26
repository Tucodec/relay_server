/***************************************
* @file     tcpserver.h
****************************************/
#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <string>
#include <list>
#include <map>
#include <vector>
#include "uv.h"
#include "PacketSync.h"
#include "SimpleUVExport.h"
#include "BaseMsgDefine.h"
#include "TcpHandle.h"
#ifndef BUFFER_SIZE
#define BUFFER_SIZE (1024*10)
#endif

/***************************************************************Server*******************************************************************************/
class AcceptClient;

template  class SUV_EXPORT std::list<TcpClientCtx*>;
template  class SUV_EXPORT std::list<write_param*>;

class SUV_EXPORT CTCPServer : public CTCPHandle
{
public:
	CTCPServer();
	virtual ~CTCPServer();

	static void  StartLog(const char* logpath = nullptr);
	static void  StopLog();

	void  SetRecvCB(int clientid, ServerRecvCB cb, void* userdata); //set recv cb. call for each accept client.

	bool  Start(const char* ip, int port);//Start the server, ipv4
	bool  Start6(const char* ip, int port);//Start the server, ipv6
	void  Close();//send close command. verify IsClosed for real closed
	virtual int  ParsePacket(const NetPacket& packet, const unsigned char* buf, TcpClientCtx *pClient);

	friend void AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	friend void AfterRecv(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
	friend void AfterSend(uv_write_t* req, int status);

protected:
	int  GetAvailaClientID()const;
	void NewConnect(int clientid);
	virtual void CloseCB(int clientid);
	virtual bool  init();
	virtual void  closeinl();//real close fun

	template<class TYPE>
	int SendUvMessage(const TYPE& msg, size_t nMsgType, TcpClientCtx *pClient);
	//Static callback function
	static void AfterServerClose(uv_handle_t* handle);
	static void DeleteTcpHandle(uv_handle_t* handle);             //delete handle after close client
	static void RecycleTcpHandle(uv_handle_t* handle);            //recycle handle after close client
	static void AcceptConnection(uv_stream_t* server, int status);
	static void SubClientClosed(int clientid, void* userdata);    //AcceptClient close cb
	static void AsyncCloseCB(uv_async_t* handle);                 //async close
	static void CloseWalkCB(uv_handle_t* handle, void* arg);      //close all handle in loop

private:
	bool bind(const char* ip, int port);
	bool bind6(const char* ip, int port);
	bool listen(int backlog = SOMAXCONN);
	bool  sendinl(const std::string& senddata, TcpClientCtx* client);
	bool broadcast(const std::string& senddata, std::vector<int> excludeid);  //broadcast to all clients, except the client who's id in excludeid
		
	static void StartThread(void* arg);  //start thread,run until use close the server
	int m_nStartsSatus;		
	int m_nServerPort;
	std::string m_nServerIP;
	uv_thread_t m_startThreadHandle;     //start thread handle

	std::list<TcpClientCtx*> m_listAvaiTcpHandle;   //Availa accept client data
	std::list<write_param*> m_listWriteParam;       //Availa write_t

protected:
	std::map<int, AcceptClient*> m_mapClientsList; //clients map
};

template<class TYPE>
int CTCPServer::SendUvMessage(const TYPE& msg, size_t nMsgType, TcpClientCtx *pClient)
{
	return this->sendinl(this->PacketData(msg, nMsgType), pClient); 
}

/***********************************************Accept client on Server**********************************************************************/
/*************************************************
Fun: The accept client on server
Usage:
Set the call back fun:      SetRecvCB/SetClosedCB
Close it             :      Close. this fun only set the close command, verify real close in the call back fun which SetRecvCB set.
GetTcpHandle         :      return the client data to server.
GetLastErrMsg        :      when the above fun call failure, call this fun to get the error message.
*************************************************/
class SUV_EXPORT AcceptClient
{
public:
	//control: accept client data. handle by server
	//loop:    the loop of server
	AcceptClient(TcpClientCtx* control, int clientid, char packhead, char packtail, uv_loop_t* loop);
	virtual ~AcceptClient();

	// void SetRecvCB(ServerRecvCB pfun, void* userdata);//set recv cb
	void SetClosedCB(TcpCloseCB pfun, void* userdata);//set close cb.
	TcpClientCtx* GetTcpHandle(void) const;

	void Close();

	const char* GetLastErrMsg() const {
		return m_strErrMsg.c_str();
	};

	friend void AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	friend void AfterRecv(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
	friend void AfterSend(uv_write_t* req, int status);
private:
	bool init(char packhead, char packtail);

	uv_loop_t* loop_;
	int m_nClientID;

	TcpClientCtx* m_pClientHandle;  //accept client data
	bool m_bIsClosed;
	std::string m_strErrMsg;

	TcpCloseCB closedcb_;
	void* closedcb_userdata_;
private:
	static void AfterClientClose(uv_handle_t* handle);
};

//Global Function
void AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void AfterRecv(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
void AfterSend(uv_write_t* req, int status);


#endif // TCPSERVER_H