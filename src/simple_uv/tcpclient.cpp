// #include "stdafx.h"
#include "tcpclient.h"
#include "UVThread.h"
// #include "log4z.h"
#define MAXLISTSIZE 20


/*****************************************TCP Client*************************************************************/
CTCPClient::CTCPClient()
    : m_nConnectStatus(CONNECT_DIS), m_buferWrite(BUFFER_SIZE)
    , m_bIsIPv6(false), m_bIsReconnecting(false), m_bTcpHandleClosed(false)
{
    m_handleClient = AllocTcpClientCtx(this);
    m_connetcReq.data = this;
}


CTCPClient::~CTCPClient()
{
    Close();
    uv_thread_join(&m_threadConnect);
    FreeTcpClientCtx(m_handleClient);
    for (auto it = m_listWriteParam.begin(); it != m_listWriteParam.end(); ++it) {
        FreeWriteParam(*it);
    }
    m_listWriteParam.clear();
}

bool CTCPClient::init()
{
	if (CTCPHandle::init())
	{
		m_handleClient->tcphandle = this->m_tcpHandle;
		uv_tcp_init(&this->m_loop, &m_handleClient->tcphandle);
		m_handleClient->tcphandle.data = m_handleClient;
		m_handleClient->parent_server = this;

		m_handleClient->packet_->SetPacketCB(GetPacket, m_handleClient);
		m_handleClient->packet_->Start(m_cPacketHead, m_cPacketTail);

		int iret = uv_timer_init(&m_loop, &m_timerReconnet);
		if (iret) {
			m_strErrMsg = GetUVError(iret);
			// // LOGI(errmsg_);
			return false;
		}
		m_timerReconnet.data = this;

		return true;
	}
	return false;
}

void CTCPClient::closeinl()
{
    if (m_bIsClosed) {
        return;
    }
    StopReconnect();
	uv_stop(&m_loop);
    uv_walk(&m_loop, CloseWalkCB, this);
}

void CTCPClient::ReConnectCB(NET_EVENT_TYPE eventtype)
{
	CTCPClient* client = this;
	if (NET_EVENT_TYPE_RECONNECT == eventtype) {
		client->AfterReconnectSuccess();
	}
	else {
		fprintf(stdout, "server disconnect.\n");
		client->AfterDisconnected();
	}
}


bool CTCPClient::Connect(const char* ip, int port)
{
    closeinl();
    if (!init()) {
        return false;
    }
    m_strServerIP = ip;
    m_nServerPort = port;
    m_bIsIPv6 = false;
    struct sockaddr_in bind_addr;
    int iret = uv_ip4_addr(m_strServerIP.c_str(), m_nServerPort, &bind_addr);
    if (iret) {
        m_strErrMsg = GetUVError(iret);
        // // LOGI(errmsg_);
        return false;
    }
    iret = uv_tcp_connect(&m_connetcReq, &m_handleClient->tcphandle, (const sockaddr*)&bind_addr, AfterConnect);
    if (iret) {
        m_strErrMsg = GetUVError(iret);
        // // LOGI(errmsg_);
        return false;
    }

    // LOGI("client(" << this << ")start connect to server(" << ip << ":" << port << ")");
    iret = uv_thread_create(&m_threadConnect, ConnectThread, this);//thread to wait for succeed connect.
    if (iret) {
        m_strErrMsg = GetUVError(iret);
        // // LOGI(errmsg_);
        return false;
    }
    int wait_count = 0;
    while (m_nConnectStatus == CONNECT_DIS) {
        ThreadSleep(100);
        if (++wait_count > 100) {
            m_nConnectStatus = CONNECT_TIMEOUT;
            break;
        }
    }
    if (CONNECT_FINISH != m_nConnectStatus) {
        m_strErrMsg = "connect time out";
        return false;
    } else {
        return true;
    }
}

bool CTCPClient::Connect6(const char* ip, int port)
{
    closeinl();
    if (!init()) {
        return false;
    }
    m_strServerIP = ip;
    m_nServerPort = port;
    m_bIsIPv6 = true;
    struct sockaddr_in6 bind_addr;
    int iret = uv_ip6_addr(m_strServerIP.c_str(), m_nServerPort, &bind_addr);
    if (iret) {
        m_strErrMsg = GetUVError(iret);
        // // LOGI(errmsg_);
        return false;
    }
    iret = uv_tcp_connect(&m_connetcReq, &m_handleClient->tcphandle, (const sockaddr*)&bind_addr, AfterConnect);
    if (iret) {
        m_strErrMsg = GetUVError(iret);
        // // LOGI(errmsg_);
        return false;
    }

    // LOGI("client(" << this << ")start connect to server(" << ip << ":" << port << ")");
    iret = uv_thread_create(&m_threadConnect, ConnectThread, this);//thread to wait for succeed connect.
    if (iret) {
        m_strErrMsg = GetUVError(iret);
        // // LOGI(errmsg_);
        return false;
    }
    int wait_count = 0;
    while (m_nConnectStatus == CONNECT_DIS) {
        ThreadSleep(100);
        if (++wait_count > 100) {
            m_nConnectStatus = CONNECT_TIMEOUT;
            break;
        }
    }
    if (CONNECT_FINISH != m_nConnectStatus) {
        m_strErrMsg = "connect time out";
        return false;
    } else {
        return true;
    }
}

void CTCPClient::ConnectThread(void* arg)
{
    CTCPClient* pclient = (CTCPClient*)arg;
    pclient->run();

}

void CTCPClient::AfterConnect(uv_connect_t* handle, int status)
{
    TcpClientCtx* theclass = (TcpClientCtx*)handle->handle->data;
    CTCPClient* parent = (CTCPClient*)theclass->parent_server;
    if (status) {
        parent->m_nConnectStatus = CONNECT_ERROR;
        (parent->m_strErrMsg) = GetUVError(status);
        // // LOGI("client(" << parent << ") connect error:" << parent->errmsg_);
        fprintf(stdout, "connect error:%s\n", parent->m_strErrMsg.c_str());
        if (parent->m_bIsReconnecting) {//reconnect failure, restart timer to trigger reconnect.
            uv_timer_stop(&parent->m_timerReconnet);
            parent->m_nRepeatTime *= 2;
            uv_timer_start(&parent->m_timerReconnet, CTCPClient::ReconnectTimer, parent->m_nRepeatTime, parent->m_nRepeatTime);
        }
        return;
    }

    int iret = uv_read_start(handle->handle, AllocBufferForRecv, AfterRecv);
    if (iret) {
        (parent->m_strErrMsg) = GetUVError(status);
        // // LOGI("client(" << parent << ") uv_read_start error:" << parent->errmsg_);
        fprintf(stdout, "uv_read_start error:%s\n", parent->m_strErrMsg.c_str());
        parent->m_nConnectStatus = CONNECT_ERROR;
    } else {
        parent->m_nConnectStatus = CONNECT_FINISH;
        // LOGI("client(" << parent << ")run");
    }
    if (parent->m_bIsReconnecting) {
        fprintf(stdout, "reconnect succeed\n");
        parent->StopReconnect();//reconnect succeed.
        
		parent->ReConnectCB(NET_EVENT_TYPE_RECONNECT);
    }
}

int CTCPClient::Send(const char* data, std::size_t len)
{
    if (!data || len <= 0) {
        m_strErrMsg = "send data is null or len less than zero.";
        // // LOGI(errmsg_);
        return 0;
    }
    uv_async_send(&m_asyncHandle);
    size_t iret = 0;
    while (!m_bIsUserAskForClosed) {
        uv_mutex_lock(&m_mutexClients);
        iret += m_buferWrite.write(data + iret, len - iret);
        uv_mutex_unlock(&m_mutexClients);
        if (iret < len) {
            ThreadSleep(100);
            continue;
        } else {
            break;
        }
    }
    uv_async_send(&m_asyncHandle);
    return iret;
}


void CTCPClient::AllocBufferForRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    TcpClientCtx* theclass = (TcpClientCtx*)handle->data;
    assert(theclass);
    *buf = theclass->read_buf_;
}

void CTCPClient::AfterRecv(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    TcpClientCtx* theclass = (TcpClientCtx*)handle->data;
    assert(theclass);
    CTCPClient* parent = (CTCPClient*)theclass->parent_server;
    if (nread < 0) {
		parent->ReConnectCB(NET_EVENT_TYPE_DISCONNECT);

        if (!parent->StartReconnect()) {
            fprintf(stdout, "Start Reconnect Failure.\n");
            return;
        }
        if (nread == UV_EOF) {
            fprintf(stdout, "Server close(EOF), Client %p\n", handle);
            // LOGI(("Server close(EOF)");
        } else if (nread == UV_ECONNRESET) {
            fprintf(stdout, "Server close(conn reset),Client %p\n", handle);
            // LOGI(("Server close(conn reset)");
        } else {
            fprintf(stdout, "Server close,Client %p:%s\n", handle, GetUVError(nread).c_str());
            // LOGI(("Server close" << GetUVError(nread));
        }
        uv_close((uv_handle_t*)handle, AfterClientClose);//close before reconnect
		parent->m_bTcpHandleClosed = true;
        return;
    }
    parent->send_inl(NULL);
    if (nread > 0) {
        // theclass->packet_->recvdata((const unsigned char*)buf->base, nread);
		parent->recvdata((const unsigned char*)buf->base, nread);
    }
}

void CTCPClient::recvdata(const unsigned char* data, size_t len)
{
	m_handleClient->packet_->recvdata(data, len);
}

void CTCPClient::AfterSend(uv_write_t* req, int status)
{
    CTCPClient* theclass = (CTCPClient*)req->data;
    if (status < 0) {
        if (theclass->m_listWriteParam.size() > MAXLISTSIZE) {
            FreeWriteParam((write_param*)req);
        } else {
            theclass->m_listWriteParam.push_back((write_param*)req);
        }
        // // LOGI("send error:" << GetUVError(status));
        fprintf(stderr, "send error %s\n", GetUVError(status).c_str());
        return;
    }
    theclass->send_inl(req);
}

/* Fully close a loop */
void CTCPClient::CloseWalkCB(uv_handle_t* handle, void* arg)
{
    CTCPClient* theclass = (CTCPClient*)arg;
    if (!uv_is_closing(handle)) {
		if (arg == handle->data)
			uv_close(handle, AfterClientClose);
		else
			uv_close(handle, nullptr);
		theclass->m_bTcpHandleClosed = true;
    }
}

void CTCPClient::AfterClientClose(uv_handle_t* handle)
{
    CTCPClient* theclass = (CTCPClient*)handle->data;
    fprintf(stdout, "Close CB handle %p\n", handle);
    if (handle == (uv_handle_t*)&theclass->m_handleClient->tcphandle && theclass->m_bIsReconnecting) {
        //closed, start reconnect timer
        int iret = 0;
        iret = uv_timer_start(&theclass->m_timerReconnet, CTCPClient::ReconnectTimer, theclass->m_nRepeatTime, theclass->m_nRepeatTime);
        if (iret) {
            uv_close((uv_handle_t*)&theclass->m_timerReconnet, CTCPClient::AfterClientClose);
            // // LOGI(GetUVError(iret));
            return;
        }
    }
}

void CTCPClient::StartLog(const char* logpath /*= nullptr*/)
{
    /*zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerMonthdir(LOG4Z_MAIN_LOGGER_ID, true);
    zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerDisplay(LOG4Z_MAIN_LOGGER_ID, false);
    zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_DEBUG);
    zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerLimitSize(LOG4Z_MAIN_LOGGER_ID, 100);
    if (logpath) {
        zsummer::log4z::ILog4zManager::GetInstance()->SetLoggerPath(LOG4Z_MAIN_LOGGER_ID, logpath);
    }
    zsummer::log4z::ILog4zManager::GetInstance()->Start();*/
}

void CTCPClient::StopLog()
{
    // zsummer::log4z::ILog4zManager::GetInstance()->Stop();
}


/*
void TCPClient::AsyncCB(uv_async_t* handle)
{
    TCPClient* theclass = (TCPClient*)handle->data;
    if (theclass->isuseraskforclosed_) {
        theclass->closeinl();
        return;
    }
    //check data to send
    theclass->send_inl(NULL);
}*/

void CTCPClient::send_inl(uv_write_t* req /*= NULL*/)
{
    write_param* writep = (write_param*)req;
    if (writep) {
        if (m_listWriteParam.size() > MAXLISTSIZE) {
            FreeWriteParam(writep);
        } else {
            m_listWriteParam.push_back(writep);
        }
    }
    while (true) {
        uv_mutex_lock(&m_mutexClients);
        if (m_buferWrite.empty()) {
            uv_mutex_unlock(&m_mutexClients);
            break;
        }
        if (m_listWriteParam.empty()) {
            writep = AllocWriteParam();
            writep->write_req_.data = this;
        } else {
            writep = m_listWriteParam.front();
            m_listWriteParam.pop_front();
        }
        writep->buf_.len = m_buferWrite.read(writep->buf_.base, writep->buf_truelen_); 
        uv_mutex_unlock(&m_mutexClients);
        int iret = uv_write((uv_write_t*)&writep->write_req_, (uv_stream_t*)&m_handleClient->tcphandle, &writep->buf_, 1, AfterSend);
        if (iret) {
            m_listWriteParam.push_back(writep);//failure not call AfterSend. so recycle req
            // // LOGI("client(" << this << ") send error:" << GetUVError(iret));
            fprintf(stdout, "send error. %s-%s\n", uv_err_name(iret), uv_strerror(iret));
        }
    }
}

int CTCPClient::ParsePacket(const NetPacket& packet, const unsigned char* buf, TcpClientCtx *pClient)
{
	return 0;
}

void CTCPClient::Close()
{
    if (m_bIsClosed) {
        return;
    }
    m_bIsUserAskForClosed = true;
    uv_async_send(&m_asyncHandle);
}

bool CTCPClient::StartReconnect(void)
{
    m_bIsReconnecting = true;
    m_handleClient->tcphandle.data = this;
    m_nRepeatTime = 1000;//1 sec
    return true;
}

void CTCPClient::StopReconnect(void)
{
    m_bIsReconnecting = false;
    m_handleClient->tcphandle.data = m_handleClient;
    m_nRepeatTime = 1000;//1 sec
    uv_timer_stop(&m_timerReconnet);
}

void CTCPClient::ReconnectTimer(uv_timer_t* handle)
{
    CTCPClient* theclass = (CTCPClient*)handle->data;
    if (!theclass->m_bIsReconnecting) {
        return;
    }
    // LOGI("start reconnect...\n");
    do {
		int iret = 0; 
		if (theclass->m_bTcpHandleClosed)
		{
			iret = uv_tcp_init(&theclass->m_loop, &theclass->m_handleClient->tcphandle);
			theclass->m_bTcpHandleClosed = false;
		}

        if (iret) {
            // // LOGI(GetUVError(iret));
            break;
        }
        theclass->m_handleClient->tcphandle.data = theclass->m_handleClient;
        theclass->m_handleClient->parent_server = theclass;
        struct sockaddr* pAddr;
        if (theclass->m_bIsIPv6) {
            struct sockaddr_in6 bind_addr;
            int iret = uv_ip6_addr(theclass->m_strServerIP.c_str(), theclass->m_nServerPort, &bind_addr);
            if (iret) {
                // // LOGI(GetUVError(iret));
                uv_close((uv_handle_t*)&theclass->m_handleClient->tcphandle, NULL);
				theclass->m_bTcpHandleClosed = true;
                break;
            }
            pAddr = (struct sockaddr*)&bind_addr;
        } else {
            struct sockaddr_in bind_addr;
            int iret = uv_ip4_addr(theclass->m_strServerIP.c_str(), theclass->m_nServerPort, &bind_addr);
            if (iret) {
                // // LOGI(GetUVError(iret));
                uv_close((uv_handle_t*)&theclass->m_handleClient->tcphandle, NULL);
				theclass->m_bTcpHandleClosed = true;
                break;
            }
            pAddr = (struct sockaddr*)&bind_addr;
        }
        iret = uv_tcp_connect(&theclass->m_connetcReq, &theclass->m_handleClient->tcphandle, (const sockaddr*)pAddr, AfterConnect);
        if (iret) {
            // // LOGI(GetUVError(iret));
            uv_close((uv_handle_t*)&theclass->m_handleClient->tcphandle, NULL);
			theclass->m_bTcpHandleClosed = true;
            break;
        }
        return;
    } while (0);
    //reconnect failure, restart timer to trigger reconnect.
    uv_timer_stop(handle);
    theclass->m_nRepeatTime *= 2;
    uv_timer_start(handle, CTCPClient::ReconnectTimer, theclass->m_nRepeatTime, theclass->m_nRepeatTime);
}

void CTCPClient::CloseCB(int clientid, void* userdata)
{
	this->Close();
}

