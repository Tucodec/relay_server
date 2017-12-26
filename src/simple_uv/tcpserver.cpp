// #include "stdafx.h"
#include "tcpserver.h"
#include "UVThread.h"
#include "LogMng.h"
#include <assert.h>
// #include "log4z.h"
#define MAXLISTSIZE 20



/*****************************************TCP Server*************************************************************/
CTCPServer::CTCPServer()
	: m_nStartsSatus(CONNECT_DIS)
{

}


CTCPServer::~CTCPServer()
{
	Close();
	uv_thread_join(&m_startThreadHandle);

	for (auto it = m_listAvaiTcpHandle.begin(); it != m_listAvaiTcpHandle.end(); ++it) {
		FreeTcpClientCtx(*it);
	}
	m_listAvaiTcpHandle.clear();

	for (auto it = m_listWriteParam.begin(); it != m_listWriteParam.end(); ++it) {
		FreeWriteParam(*it);
	}
	m_listWriteParam.clear();
	//    // LOGI("tcp server exit.");
}

bool CTCPServer::init()
{
	CTCPHandle::init();
	return uv_tcp_nodelay(&m_tcpHandle,  1) == 0;
}

void CTCPServer::closeinl()
{
	if (m_bIsClosed) {
		return;
	}

	uv_mutex_lock(&m_mutexClients);
	for (auto it = m_mapClientsList.begin(); it != m_mapClientsList.end(); ++it) {
		auto data = it->second;
		data->Close();
	}
	uv_walk(&m_loop, CloseWalkCB, this);//close all handle in loop
	//    // LOGI("close server");
}

bool CTCPServer::bind(const char* ip, int port)
{
	struct sockaddr_in bind_addr;
	int iret = uv_ip4_addr(ip, port, &bind_addr);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		//        // LOGE(errmsg_);
		return false;
	}
	iret = uv_tcp_bind(&m_tcpHandle, (const struct sockaddr*)&bind_addr, 0);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		//        // LOGE(errmsg_);
		return false;
	}
	//    // LOGI("server bind ip=" << ip << ", port=" << port);
	return true;
}

bool CTCPServer::bind6(const char* ip, int port)
{
	struct sockaddr_in6 bind_addr;
	int iret = uv_ip6_addr(ip, port, &bind_addr);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		//        // LOGE(errmsg_);
		return false;
	}
	iret = uv_tcp_bind(&m_tcpHandle, (const struct sockaddr*)&bind_addr, 0);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		//        // LOGE(errmsg_);
		return false;
	}

	//    // LOGI("server bind ip=" << ip << ", port=" << port);
	return true;
}

bool CTCPServer::listen(int backlog)
{
	int iret = uv_listen((uv_stream_t*) &m_tcpHandle, backlog, AcceptConnection);
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		//        // LOGE(errmsg_);
		return false;
	}
	//    // LOGI("server Start listen. Runing.......");
	fprintf(stdout, "Server Runing.......\n");
	return true;
}

bool CTCPServer::Start(const char* ip, int port)
{
	m_nServerIP = ip;
	m_nServerPort = port;
	closeinl();
	if (!init()) {
		return false;
	}
	if (!bind(m_nServerIP.c_str(), m_nServerPort)) {
		return false;
	}
	if (!listen(SOMAXCONN)) {
		return false;
	}
	int iret = uv_thread_create(&m_startThreadHandle, StartThread, this);//use thread to wait for start succeed.
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		//        // LOGE(errmsg_);
		return false;
	}
	int wait_count = 0;
	while (m_nStartsSatus == CONNECT_DIS) {
		ThreadSleep(100);
		if (++wait_count > 100) {
			m_nStartsSatus = CONNECT_TIMEOUT;
			break;
		}
	}
	return m_nStartsSatus == CONNECT_FINISH;
}

bool CTCPServer::Start6(const char* ip, int port)
{
	m_nServerIP = ip;
	m_nServerPort = port;
	closeinl();
	if (!init()) {
		return false;
	}
	if (!bind6(m_nServerIP.c_str(), m_nServerPort)) {
		return false;
	}
	if (!listen(SOMAXCONN)) {
		return false;
	}
	int iret = uv_thread_create(&m_startThreadHandle, StartThread, this);//use thread to wait for start succeed.
	if (iret) {
		m_strErrMsg = GetUVError(iret);
		//        // LOGE(errmsg_);
		return false;
	}
	int wait_count = 0;
	while (m_nStartsSatus == CONNECT_DIS) {
		ThreadSleep(100);
		if (++wait_count > 100) {
			m_nStartsSatus = CONNECT_TIMEOUT;
			break;
		}
	}
	return m_nStartsSatus == CONNECT_FINISH;
}

void CTCPServer::StartThread(void* arg)
{
	CTCPServer* theclass = (CTCPServer*)arg;
	theclass->m_nStartsSatus = CONNECT_FINISH;
	theclass->run();
	//the server is close when come here
	theclass->m_bIsClosed = true;
	theclass->m_bIsUserAskForClosed = false;
	//    // LOGI("server  had closed.");
	//     if (theclass->closedcb_) {//trigger the close cb
	//         theclass->closedcb_(-1, theclass->closedcb_userdata_);
	//     }
	theclass->CloseCB(-1);
}

void CTCPServer::AcceptConnection(uv_stream_t* server, int status)
{
	CTCPServer* tcpsock = (CTCPServer*)server->data;
	assert(tcpsock);
	if (status) {
		(tcpsock->m_strErrMsg) = GetUVError(status);
		//        // LOGE(tcpsock->errmsg_);
		return;
	}
	TcpClientCtx* tmptcp = NULL;
	if (tcpsock->m_listAvaiTcpHandle.empty()) {
		tmptcp = AllocTcpClientCtx(tcpsock);
	} else {
		tmptcp = tcpsock->m_listAvaiTcpHandle.front();
		tcpsock->m_listAvaiTcpHandle.pop_front();
		tmptcp->parent_acceptclient = NULL;
	}
	int iret = uv_tcp_init(&tcpsock->m_loop, &tmptcp->tcphandle);
	if (iret) {
		tcpsock->m_listAvaiTcpHandle.push_back(tmptcp);//Recycle
		(tcpsock->m_strErrMsg) = GetUVError(iret);
		//        // LOGE(tcpsock->errmsg_);
		return;
	}
	tmptcp->tcphandle.data = tmptcp;

	auto clientid = tcpsock->GetAvailaClientID();
	tmptcp->clientid = clientid;
	iret = uv_accept((uv_stream_t*)server, (uv_stream_t*)&tmptcp->tcphandle);
	if (iret) {
		tcpsock->m_listAvaiTcpHandle.push_back(tmptcp);//Recycle
		(tcpsock->m_strErrMsg) = GetUVError(iret);
		//        // LOGE(tcpsock->errmsg_);
		return;
	}
	tmptcp->packet_->SetPacketCB(GetPacket, tmptcp);
	tmptcp->packet_->Start(tcpsock->m_cPacketHead, tcpsock->m_cPacketTail);
	iret = uv_read_start((uv_stream_t*)&tmptcp->tcphandle, AllocBufferForRecv, AfterRecv);
	if (iret) {
		uv_close((uv_handle_t*)&tmptcp->tcphandle, CTCPServer::RecycleTcpHandle);
		(tcpsock->m_strErrMsg) = GetUVError(iret);
		// LOGE(tcpsock->errmsg_);
		return;
	}
	AcceptClient* cdata = new AcceptClient(tmptcp, clientid, tcpsock->m_cPacketHead, tcpsock->m_cPacketTail, &tcpsock->m_loop); //delete on SubClientClosed
	cdata->SetClosedCB(CTCPServer::SubClientClosed, tcpsock);
	uv_mutex_lock(&tcpsock->m_mutexClients);
	tcpsock->m_mapClientsList.insert(std::make_pair(clientid, cdata)); //add accept client
	uv_mutex_unlock(&tcpsock->m_mutexClients);

	tcpsock->NewConnect(clientid);
	// LOGI("new client id=" << clientid);
	return;
}

void CTCPServer::SetRecvCB(int clientid, ServerRecvCB cb, void* userdata)
{
	//     uv_mutex_lock(&mutex_clients_);
	//     auto itfind = m_mapClientsList->find(clientid);
	//     if (itfind != m_mapClientsList->end()) {
	//         itfind->second->SetRecvCB(cb, userdata);
	//     }
	//     uv_mutex_unlock(&mutex_clients_);
}


/* Fully close a loop */
void CTCPServer::CloseWalkCB(uv_handle_t* handle, void* arg)
{
	CTCPServer* theclass = (CTCPServer*)arg;
	if (!uv_is_closing(handle)) {
		uv_close(handle, AfterServerClose);
	}
}

void CTCPServer::AfterServerClose(uv_handle_t* handle)
{
	CTCPServer* theclass = (CTCPServer*)handle->data;
	fprintf(stdout, "Close CB handle %p\n", handle);
}

void CTCPServer::DeleteTcpHandle(uv_handle_t* handle)
{
	TcpClientCtx* theclass = (TcpClientCtx*)handle->data;
	FreeTcpClientCtx(theclass);
}

void CTCPServer::RecycleTcpHandle(uv_handle_t* handle)
{
	//the handle on TcpClientCtx had closed.
	TcpClientCtx* theclass = (TcpClientCtx*)handle->data;
	assert(theclass);
	CTCPServer* parent = (CTCPServer*)theclass->parent_server;
	if (parent->m_listAvaiTcpHandle.size() > MAXLISTSIZE) {
		FreeTcpClientCtx(theclass);
	} else {
		parent->m_listAvaiTcpHandle.push_back(theclass);
	}
}

int CTCPServer::GetAvailaClientID() const
{
	static int s_id = 0;
	return ++s_id;
}

void CTCPServer::NewConnect(int clientid)
{
	fprintf(stdout, "new connect:%d\n", clientid);
	this->SetRecvCB(clientid, NULL, NULL);
}

void CTCPServer::CloseCB(int clientid)
{
	fprintf(stdout, "cliend %d close\n", clientid);
}

void CTCPServer::StartLog(const char* logpath /*= nullptr*/)
{
	/*zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerMonthdir(LOG4Z_MAIN_LOGGER_ID, true);
	zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerDisplay(LOG4Z_MAIN_LOGGER_ID, true);
	zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_DEBUG);
	zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerLimitSize(LOG4Z_MAIN_LOGGER_ID, 100);
	if (logpath) {
	zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerPath(LOG4Z_MAIN_LOGGER_ID, logpath);
	}
	zsummer::log4z::ILog4zManager::GetInstance()->Start();*/
}

void CTCPServer::StopLog()
{
	// zsummer::log4z::ILog4zManager::GetInstance()->Stop();
}

void CTCPServer::SubClientClosed(int clientid, void* userdata)
{
	CTCPServer* theclass = (CTCPServer*)userdata;
	uv_mutex_lock(&theclass->m_mutexClients);
	auto itfind = theclass->m_mapClientsList.find(clientid);
	if (itfind != theclass->m_mapClientsList.end()) {
		theclass->CloseCB(clientid);
		if (theclass->m_listAvaiTcpHandle.size() > MAXLISTSIZE) {
			FreeTcpClientCtx(itfind->second->GetTcpHandle());
		} else {
			theclass->m_listAvaiTcpHandle.push_back(itfind->second->GetTcpHandle());
		}
		delete itfind->second;
		// LOGI("delete client:" << itfind->first);
		fprintf(stdout, "delete client：%d\n", itfind->first);
		theclass->m_mapClientsList.erase(itfind);
	}
	uv_mutex_unlock(&theclass->m_mutexClients);
}

void CTCPServer::AsyncCloseCB(uv_async_t* handle)
{
	CTCPServer* theclass = (CTCPServer*)handle->data;
	if (theclass->m_bIsUserAskForClosed) {
		theclass->closeinl();
	}
	return;
}

void CTCPServer::Close()
{
	if (m_bIsClosed) {
		return;
	}
	m_bIsUserAskForClosed = true;
	uv_async_send(&m_asyncHandle);
}

bool CTCPServer::broadcast(const std::string& senddata, std::vector<int> excludeid)
{
	if (senddata.empty()) {
		// LOGI("broadcast data is empty.");
		return true;
	}
	uv_mutex_lock(&m_mutexClients);
	AcceptClient* pClient = NULL;
	write_param* writep = NULL;
	if (excludeid.empty()) {
		for (auto it = m_mapClientsList.begin(); it != m_mapClientsList.end(); ++it) {
			pClient = it->second;
			sendinl(senddata, pClient->GetTcpHandle());
		}
	} else {
		for (auto it = m_mapClientsList.begin(); it != m_mapClientsList.end(); ++it) {
			auto itfind = std::find(excludeid.begin(), excludeid.end(), it->first);
			if (itfind != excludeid.end()) {
				excludeid.erase(itfind);
				continue;
			}
			pClient = it->second;
			sendinl(senddata, pClient->GetTcpHandle());
		}
	}
	uv_mutex_unlock(&m_mutexClients);
	return true;
}

bool CTCPServer::sendinl(const std::string& senddata, TcpClientCtx* client)
{
	if (senddata.empty()) {
		// LOGI("send data is empty.");
		return true;
	}
	write_param* writep = NULL;
	if (m_listWriteParam.empty()) {
		writep = AllocWriteParam();
	} else {
		writep = m_listWriteParam.front();
		m_listWriteParam.pop_front();
	}
	if (writep->buf_truelen_ < senddata.length()) {
		writep->buf_.base = (char*)realloc(writep->buf_.base, senddata.length());
		writep->buf_truelen_ = senddata.length();
	}
	memcpy(writep->buf_.base, senddata.data(), senddata.length());
	writep->buf_.len = senddata.length();
	writep->write_req_.data = client;
	int iret = uv_write((uv_write_t*)&writep->write_req_, (uv_stream_t*)&client->tcphandle, &writep->buf_, 1, AfterSend);//发送
	if (iret) {
		m_listWriteParam.push_back(writep);
		m_strErrMsg = "send data error.";
		// LOGE("client(" << client << ") send error:" << GetUVError(iret));
		fprintf(stdout, "send error. %s-%s\n", uv_err_name(iret), uv_strerror(iret));
		return false;
	}
	return true;
}

int CTCPServer::ParsePacket( const NetPacket& packet, const unsigned char* buf, TcpClientCtx *pClient)
{
	return 0;
}
// 
// int TCPServer::SendUvMessage( TcpClientCtx *pClient, const char *pData, size_t nSize)
// {
// 	NetPacket tmppack;
// 	tmppack.header = SERVER_PACKET_HEAD;
// 	tmppack.tail = SERVER_PACKET_TAIL;
// 	tmppack.datalen = nSize;
// 	std::string pro_packet_ = PacketData2(tmppack,(const unsigned char*)pData);
// 	return this->sendinl(pro_packet_, pClient);
// }

/*****************************************AcceptClient*************************************************************/
AcceptClient::AcceptClient(TcpClientCtx* control,  int clientid, char packhead, char packtail, uv_loop_t* loop)
	: m_pClientHandle(control)
	, m_nClientID(clientid), loop_(loop)
	, m_bIsClosed(true)
	// , recvcb_(nullptr), recvcb_userdata_(nullptr)
	, closedcb_(nullptr), closedcb_userdata_(nullptr)
{
	init(packhead, packtail);
}

AcceptClient::~AcceptClient()
{
	Close();
	//while will block loop.
	//the right way is new AcceptClient and delete it on SetClosedCB'cb
	while (!m_bIsClosed) {
		ThreadSleep(10);
	}
}

bool AcceptClient::init(char packhead, char packtail)
{
	if (!m_bIsClosed) {
		return true;
	}
	m_pClientHandle->parent_acceptclient = this;
	m_bIsClosed = false;
	return true;
}

void AcceptClient::Close()
{
	if (m_bIsClosed) {
		return;
	}
	m_pClientHandle->tcphandle.data = this;
	//send close command
	uv_close((uv_handle_t*)&m_pClientHandle->tcphandle, AfterClientClose);
	// LOGI("client(" << this << ")close");
}

void AcceptClient::AfterClientClose(uv_handle_t* handle)
{
	AcceptClient* theclass = (AcceptClient*)handle->data;
	assert(theclass);
	if (handle == (uv_handle_t*)&theclass->m_pClientHandle->tcphandle) {
		theclass->m_bIsClosed = true;
		// LOGI("client  had closed.");
		if (theclass->closedcb_) {//notice tcpserver the client had closed
			theclass->closedcb_(theclass->m_nClientID, theclass->closedcb_userdata_);
		}
	}
}


void AcceptClient::SetClosedCB(TcpCloseCB pfun, void* userdata)
{
	//AfterRecv trigger this cb
	closedcb_ = pfun;
	closedcb_userdata_ = userdata;
}

TcpClientCtx* AcceptClient::GetTcpHandle(void) const
{
	return m_pClientHandle;
}

/*****************************************Global*************************************************************/
void AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	TcpClientCtx* theclass = (TcpClientCtx*)handle->data;
	assert(theclass);
	*buf = theclass->read_buf_;
}

void AfterRecv(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	TcpClientCtx* theclass = (TcpClientCtx*)handle->data;
	assert(theclass);
	if (nread < 0) {/* Error or EOF */
		if (nread == UV_EOF) {
			fprintf(stdout, "client(%d)eof\n", theclass->clientid);
			// LOGI(("client(" << theclass->clientid << ")eof");
		} else if (nread == UV_ECONNRESET) {
			fprintf(stdout, "client(%d)conn reset\n", theclass->clientid);
			// LOGI(("client(" << theclass->clientid << ")conn reset");
		} else {
			fprintf(stdout, "%s\n", GetUVError(nread).c_str());
			// LOGI(("client(" << theclass->clientid << ")：" << GetUVError(nread));
		}
		AcceptClient* acceptclient = (AcceptClient*)theclass->parent_acceptclient;
		acceptclient->Close();
		return;
	} else if (0 == nread)  {/* Everything OK, but nothing read. */

	} else {
		theclass->packet_->recvdata((const unsigned char*)buf->base, nread);
	}
}

void AfterSend(uv_write_t* req, int status)
{
	TcpClientCtx* theclass = (TcpClientCtx*)req->data;
	CTCPServer* parent = (CTCPServer*)theclass->parent_server;
	if (parent->m_listWriteParam.size() > MAXLISTSIZE) {
		FreeWriteParam((write_param*)req);
	} else {
		parent->m_listWriteParam.push_back((write_param*)req);
	}
	if (status < 0) {
		// LOGE("send data error:" << GetUVError(status));
		fprintf(stderr, "send error %s.%s\n", uv_err_name(status), uv_strerror(status));
	}
}

