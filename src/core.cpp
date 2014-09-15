#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hiredis/hiredis.h>

#include <map>

#include "define.h"
#include "core.h"
#include "conf.h"
#include "template.h"


extern Conf_t g_conf;

int server_init() {
	return 0;
}

void server_ctx_clean(conn_ctx_t *ctx) {
  if (ctx && ctx->data) {
	  DEBUG_LOG("pre clean server ctx");
    redisFree((redisContext *)(ctx->data));
    ctx->data = NULL;
  }
}

redisContext *get_redis_ctx(conn_ctx_t *ctx) {
  if (!ctx->data) {
    redisContext *redis_ctx = redisConnect("127.0.0.1", 6379);
    if (redis_ctx != NULL && redis_ctx->err) {
      DEBUG_LOG("redis connect error: %s", redis_ctx->errstr);
      redisFree(redis_ctx);
      redis_ctx = NULL;
    }
    ctx->data = (void *)redis_ctx;
  }
  return (redisContext *)(ctx->data);
}

void empty_response_buf(uv_buf_t *buf) {
  buf->base[0] = '\0';
  buf->len = 0;
}

int setup_upstream_request(web_request_t *web, upstream_request_t *up)
{
	assert(web != NULL);
	assert(up != NULL);
	memcpy(up->query, web->query, sizeof(up->query));
	up->magic = sizeof(upstream_request_t);
	up->slot_id = web->slot_id;
	up->need_merge = web->need_merge;
	up->need_pb = web->need_pb;
	return 0;
}

int empty_response(conn_ctx_t *ctx) {
	char *buf = ctx->response_buf.base;
	buf[0] = '{';
	buf[1] = '}';
	buf[2] = '\0';
	ctx->response_buf.len = 2;
	return 0;
}

int worker_handler(conn_ctx_t *ctx) {

	empty_response_buf(&ctx->response_buf);
	char *buf = ctx->response_buf.base;
	int len = 0;
	int rv;
	if (ctx->upstream_response.len < 1) {
		//return empty_response(ctx);
	}

	// TODO merge

	int tn_index;
	int iserr = 0;
	if (ctx->request.web.need_pb) {
#ifdef DEBUG
		if (ctx->upstream_response.head.data.type < 1) {
			ctx->upstream_response.head.data.type = 1;
		}
#endif
		snprintf(ctx->request.up.tn, sizeof(ctx->request.up.tn), "%u", ctx->upstream_response.head.data.tpl);
		tn_index = temp_get_template_index(ctx->request.up.tn);
		if (tn_index < 0) {
			WARNING_LOG("notn[%s]", ctx->request.up.tn);
			return empty_response(ctx);
		}

		rv = temp_make_page(tn_index, buf, RESPONSE_BUF_SIZE, ctx, iserr);
		if (rv < 0) {
			rv = temp_make_page(tn_index, buf, RESPONSE_BUF_SIZE, ctx, 1);
		}
		len = rv;
		if (len < 0) {
			len = 0;
		}
	} else {
		memcpy(buf, &ctx->upstream_response.buf, ctx->upstream_response.len);
		len = ctx->upstream_response.len;
	}

	ctx->response_buf.len = len;
	return 1;
}
