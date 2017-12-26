// #include "stdafx.h"
#include "BaseMsgDefine.h"

TcpClientCtx* AllocTcpClientCtx(void* parentserver)
{
	TcpClientCtx* ctx = (TcpClientCtx*)malloc(sizeof(*ctx));
	ctx->packet_ = new CPacketSync;
	ctx->read_buf_.base = (char*)malloc(BUFFER_SIZE);
	ctx->read_buf_.len = BUFFER_SIZE;
	ctx->parent_server = parentserver;
	ctx->parent_acceptclient = NULL;
	return ctx;
}

void FreeTcpClientCtx(TcpClientCtx* ctx)
{
	delete ctx->packet_;
	free(ctx->read_buf_.base);
	free(ctx);
}

write_param* AllocWriteParam(void)
{
	write_param* param = (write_param*)malloc(sizeof(write_param));
	param->buf_.base = (char*)malloc(BUFFER_SIZE);
	param->buf_.len = BUFFER_SIZE;
	param->buf_truelen_ = BUFFER_SIZE;
	return param;
}

void FreeWriteParam(write_param* param)
{
	free(param->buf_.base);
	free(param);
}
