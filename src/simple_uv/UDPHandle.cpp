#include "UDPHandle.h"
#include "UVThreadMng.h"

CUDPHandle::CUDPHandle(uv_loop_t *pLoop):
	m_nSendByteCount(0),
	m_nRecvByteCount(0),
	m_pLoop(pLoop)
{
	m_nTcpHandle = 1;
	int iret;
	uv_async_init(m_pLoop, &m_asyncHandleForRecvMsg, AsyncRecvMsg);
	m_asyncHandleForRecvMsg.data = this;

	iret = uv_udp_init(m_pLoop, &m_udpHandle);
	if (iret) {
		LOGFMTF("init udp socket error");
	}
	m_udpHandle.data = this;
}

CUDPHandle::~CUDPHandle(void)
{
	//Close();
}

void CUDPHandle::Close()
{
	uv_udp_recv_stop(&m_udpHandle);
}

bool CUDPHandle::bind(const char* ip, int port)
{
	struct sockaddr_in bind_addr;
	int iret = uv_ip4_addr(ip, port, &bind_addr);
	if (iret) {
		LOGFMTF("IP,PORT %s:%d bind error", ip, port);
		return false;
	}
	iret = uv_udp_bind(&m_udpHandle, (const struct sockaddr*)&bind_addr, 0);
	if (iret) {
		LOGFMTF("IP,PORT %s:%d bind error", ip, port);
		return false;
	}
	return true;
}

bool CUDPHandle::bind6(const char* ip, int port)
{
	struct sockaddr_in6 bind_addr;
	int iret = uv_ip6_addr(ip, port, &bind_addr);
	if (iret) {
		return false;
	}
	iret = uv_udp_bind(&m_udpHandle, (const struct sockaddr*)&bind_addr, 0);
	if (iret) {
		return false;
	}
	return true;
}

bool CUDPHandle::setBroadcast(bool enable)
{
	int iret = uv_udp_set_broadcast(&m_udpHandle, enable);
	if (iret) {
		LOGFMTF("set udp socket broadcast error");
		return false;
	}
	return true;
}

bool CUDPHandle::setSocketBufSize(int recvBufSize,int sendBufSize)
{
	int r;
#ifdef WIN32
	/*int old_recv_size, old_recv_opt_len = sizeof(old_recv_opt_len);
	int old_send_size, old_send_opt_len = sizeof(old_recv_opt_len);
	r = getsockopt(this->m_udpHandle.socket, SOL_SOCKET, SO_RCVBUF, (char *)&old_recv_size, &old_recv_opt_len);
	r = getsockopt(this->m_udpHandle.socket, SOL_SOCKET, SO_SNDBUF, (char *)&old_send_size, &old_send_opt_len);
	printf("old buf size,recv:%d,send:%d\n", old_recv_size, old_send_size);*/
	int optlen = sizeof(recvBufSize);
	r = setsockopt(this->m_udpHandle.socket, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufSize, optlen);
	r = setsockopt(this->m_udpHandle.socket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, optlen);

	/*r = getsockopt(this->m_udpHandle.socket, SOL_SOCKET, SO_RCVBUF, (char *)&old_recv_size, &old_recv_opt_len);
	r = getsockopt(this->m_udpHandle.socket, SOL_SOCKET, SO_SNDBUF, (char *)&old_send_size, &old_send_opt_len);
	printf("new buf size,recv:%d,send:%d\n", old_recv_size, old_send_size);*/
#else
	/*unsigned int old_recv_size, old_recv_opt_len = sizeof(old_recv_opt_len);
	unsigned int old_send_size, old_send_opt_len = sizeof(old_recv_opt_len);
	r = getsockopt(this->m_udpHandle.io_watcher.fd, SOL_SOCKET, SO_RCVBUF, &old_recv_size, &old_recv_opt_len);
	r = getsockopt(this->m_udpHandle.io_watcher.fd, SOL_SOCKET, SO_SNDBUF, &old_send_size, &old_send_opt_len);
	printf("old buf size,recv:%d,send:%d\n", old_recv_size, old_send_size);*/
	int optlen = sizeof(recvBufSize);
	r = setsockopt(this->m_udpHandle.io_watcher.fd, SOL_SOCKET, SO_RCVBUF, &recvBufSize, optlen);
	r = setsockopt(this->m_udpHandle.io_watcher.fd, SOL_SOCKET, SO_SNDBUF, &sendBufSize, optlen);

	/*r = getsockopt(this->m_udpHandle.io_watcher.fd, SOL_SOCKET, SO_RCVBUF, &old_recv_size, &old_recv_opt_len);
	r = getsockopt(this->m_udpHandle.io_watcher.fd, SOL_SOCKET, SO_SNDBUF, &old_send_size, &old_send_opt_len);
	printf("new buf size,recv:%d,send:%d\n", old_recv_size, old_send_size);*/
#endif
	return r == 0;
}

bool CUDPHandle::send(const char * msg, const unsigned int msg_len, const sockaddr_in * send_addr)
{
	uv_udp_send_t *send_req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
#ifdef WIN32
	char*data =const_cast<char*>(msg);
#else
	char *data = (char*)malloc(msg_len);
	memcpy(data, msg, msg_len);
#endif
	uv_buf_t buffer = uv_buf_init(data, msg_len);
	send_req->data = data;
	int iret = uv_udp_send(send_req, &m_udpHandle, &buffer, 1, (const struct sockaddr *)send_addr, AfterUdpSend);
	m_nSendByteCount += msg_len;
	if (iret) {
		LOGFMTE("send error. %s-%s", uv_err_name(iret), uv_strerror(iret));
		return false;
	}
	return true;
}

void CUDPHandle::AsyncRecvMsg(uv_async_t* handle)
{
	CUDPHandle* pClass = (CUDPHandle*)handle->data;
	pClass->DispatchThreadMsg();
}

void CUDPHandle::OnUdpRead(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
	if (nread < 0) {
		if (nread != UV_EOF) {
			fprintf(stderr, "Read error %s\n", uv_err_name(nread));
		}
		//uv_close((uv_handle_t*)handle, OnHandleClose);
		return;
	}
	if (nread == 0) {
		if (addr == NULL)
			;//LOGFMTI("nothing to read");
		else
			LOGFMTI("Read empty data");
		return;
	}
	if (addr == NULL) {
		LOGFMTE("Read not empty data from null,error");
		return;
	}

	if (nread < 6) {
		LOGFMTE("receive data smaller then packet");
		return;
	}
	CUDPHandle *parent = reinterpret_cast<CUDPHandle*>(handle->data);
	parent->m_nRecvByteCount += nread;
	parent->ParsePacket(*reinterpret_cast<const UdpPacket*>(buf->base), buf->base, nread, addr);
}

void CUDPHandle::AllocBufferForUdpRecv(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
	static char str[65536];
	buf->base = str;
	buf->len = 65536;
}
void CUDPHandle::OnHandleClose(uv_handle_t* handle) {

}
void CUDPHandle::AfterUdpSend(uv_udp_send_t* req, int status) {
#ifdef WIN32
#else
	free(req->data);
#endif
	free(req);
}