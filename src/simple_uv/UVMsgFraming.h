#ifndef UV_MSG_FRAMING_H
#define UV_MSG_FRAMING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uv.h"
#include "SimpleUVExport.h"


typedef struct SUV_EXPORT uv_msg_s        uv_msg_t;
typedef struct SUV_EXPORT uv_msg_write_s  uv_msg_write_t;


/* Stream Initialization */

int SUV_EXPORT uv_msg_init(uv_loop_t* loop, uv_msg_t* handle, int stream_type);


/* Callback Functions */

typedef void SUV_EXPORT (*uv_free_cb)(uv_handle_t* handle, void* ptr);

typedef void SUV_EXPORT (*uv_msg_read_cb)(uv_stream_t* stream, void *msg, int size);

typedef void SUV_EXPORT (*uv_msg_write_cb)(uv_write_t *req, int status);


/* Functions */

int SUV_EXPORT uv_msg_read_start(uv_msg_t* stream, uv_alloc_cb alloc_cb, uv_msg_read_cb msg_read_cb, uv_free_cb free_cb);

int SUV_EXPORT uv_msg_send(uv_msg_write_t *req, uv_stream_t* stream, void *msg, int size, uv_write_cb write_cb);


/* Message Read Structure */

struct SUV_EXPORT uv_msg_s {
   union {
      uv_tcp_t tcp;
      uv_pipe_t pipe;
      void *data;
   };
   char *buf;
   int alloc_size;
   int filled;
   uv_alloc_cb alloc_cb;
   uv_free_cb free_cb;
   uv_msg_read_cb msg_read_cb;
};


/* Message Write Structure */

struct SUV_EXPORT uv_msg_write_s {
   union {
      uv_write_t req;
      void *data;
   };
   uv_buf_t buf[2];
   int msg_size;     /* in network order! */
};


struct NodeMsg
{
	unsigned int m_nMsgType;
	unsigned int m_nSrcAddr;
	void *m_pData;
	NodeMsg *next;
	~NodeMsg() {
		next = nullptr;
	}
};


#ifdef __cplusplus
}
#endif
#endif  // UV_MSG_FRAMING_H
