#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <hiredis/hiredis.h>

#include <map>

#include "define.h"
#include "core.h"
#include "conf.h"


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
	up->need_merge = web->need_merge;
	up->need_pb = web->need_pb;
	return 0;
}

/*

void *service_thread(void *pti)
{
	int sock = 0;
	int s_index = 0;
	int ret = 0;
	TySock_head_t shead;

	u_int max_read_size = 1024*1024;
	u_int max_send_size = 1024*1024;
	char *send_buf = (char *)malloc(max_send_size);

	// connect all the server
	Group_sock_t gs;
	server_connect_all(&gs);

	// user log info
	Session_t loginfo;

	int group_id = 0;
	int server_sock = -1;

	upstream_request_t upstream_request;//ui2.0传递给AS搜索引擎的讲求数据结构
	upstream_response_head_t upstream_response;//AS搜索返回的结果头
	web_request_t as_apache_request;//apache的搜索请求结构
	result_node_t upstream_result[MAX_SEARCH_RES_NUM];//AS搜索返回的结果数据

	int iserr = 0;
	int server_retry = 0;

	struct timeval tvstart, tvend, tv1, tv2;
	u_int timeused;

	while (1) {
		shead.length = 0;
		upstream_response.total_num = 0;
		upstream_response.return_num= 0;

		memset(&loginfo, 0, sizeof(Session_t));
		loginfo.status = OK;
		loginfo.tn_index = -1;

		// get connect
		DEBUG_LOG("before fetch");
		GetTimeCurrent(tv1);
		GetTimeCurrent(tv2);

		memset(&shead, 0, sizeof(TySock_head_t));
		group_id = 0;
		server_sock = -1;
		iserr = 0;
		server_retry = 0;

		read_buf[0] = 0;

		DEBUG_LOG("before recv apache head");
		ret = ty_net_recv(sock, &shead, sizeof(TySock_head_t), g_conf.read_tmout);
		if ((u_int)ret != sizeof(TySock_head_t)) {
			WARNING_LOG("recv head from apache failed [%d/%u]", ret, sizeof(TySock_head_t));
			loginfo.status = APACHE_GET_ERROR;
			goto message_end;
		}

		if (shead.other != (u_int)g_conf.magic_ui_num) {
			WARNING_LOG("recv head magic num error [%u - %u]", shead.other, g_conf.magic_ui_num);
			loginfo.status = APACHE_MAGIC_ERROR;
			goto message_end;
		}

		if (shead.length > max_read_size) {
			WARNING_LOG("recv leng[%u > %u] is too big", shead.length, max_read_size);
			loginfo.status = APACHE_GET_ERROR;
			goto message_end;
		}

		DEBUG_LOG("before recv apache body");
		ret = ty_net_recv(sock, read_buf, shead.length, g_conf.read_tmout);
		if ((u_int)ret != shead.length) {
			WARNING_LOG("recv body from apache failed [%d/%u]", ret, shead.length);
			loginfo.status = APACHE_GET_ERROR;
			goto message_end;
		}
		DEBUG_LOG("after recv apache body");
		GetTimeCurrent(tvstart);

		loginfo.ip = shead.ip;

		WARNING_LOG("fetch_item sock = %d, utime[%u] ip:%u.%u.%u.%u", sock, loginfo.fetch_timeused,
				shead.ip %256, (shead.ip>>8) %256, (shead.ip>>16) %256, (shead.ip>>24) %256);
		//check ip
		if (shead.need_merge == 1 || shead.need_pb == 1) {
			if (shead.length != sizeof(web_request_t)) {
				WARNING_LOG("recv leng[%d] != sizeof(web_request_t)", shead.length);

				loginfo.status = APACHE_GET_ERROR;
				goto message_end;
			}

			memcpy(&as_apache_request, read_buf, shead.length);
			set_to_upstream_request(&as_apache_request,&upstream_request);
		}

		if (shead.need_merge == 1) {
			goto merge_result;
		}

		// get server group
		group_id = server_get_group_id(shead.trace[0]);
		if (group_id < 0) {
			WARNING_LOG("server_get_group_id failed!");
			loginfo.status = APACHE_REFERER_ERROR;
			shead.status = APACHE_REFERER_ERROR;
			goto message_end;
		}

		// get server
server_work:

		if (g_conf.serv_type != SERV_TYPE_FILTER) {
			WARNING_LOG("recv req type err, not SERV_TYPE_FILTER");
			loginfo.status = APACHE_REQ_TYPE_ERROR;
			goto message_end;
		}

		server_sock = server_get_sock(&group_id, &(loginfo.server_id), &gs);
		if (server_sock < 0) {
			WARNING_LOG("there is no server sock to use");
			loginfo.status = SRV_CONN_ERROR;
			shead.status = SRV_CONN_ERROR;
			goto send_to_apache;
		}

		loginfo.group_id = group_id;
		shead.trace[1] = group_id;   //ui is the trace 1

		// send message to server
		GetTimeCurrent(tv1);
		ret = ty_net_send(server_sock, &shead, sizeof(TySock_head_t));
		if ((u_int)ret != sizeof(TySock_head_t)) {
			WARNING_LOG("send head to server[%d_%d] failed [%d/%u]",
				group_id, loginfo.server_id, ret, sizeof(TySock_head_t));
			server_err_record(group_id, loginfo.server_id, server_sock, &gs);
			if (server_retry == 0) {
				server_retry++;
				goto server_work;
			}
			loginfo.server_status = SRV_PUT_ERROR;
			loginfo.status = SRV_PUT_ERROR;
			shead.status = SRV_PUT_ERROR;
			goto send_to_apache;
		}

		ret = ty_net_send(server_sock, read_buf, shead.length);
		if ((u_int)ret != shead.length) {
			WARNING_LOG("send body to server[%d_%d] failed [%d/%u]",
				group_id, loginfo.server_id, ret, shead.length);
			server_err_record(group_id, loginfo.server_id, server_sock, &gs);
			if (server_retry == 0) {
				server_retry++;
				goto server_work;
			}
			loginfo.server_status = SRV_PUT_ERROR;
			loginfo.status = SRV_PUT_ERROR;
			shead.status = SRV_PUT_ERROR;
			goto send_to_apache;
		}

		// read result from
		memset(read_buf, 0, max_read_size);
		memset(&shead, 0, sizeof(TySock_head_t));
		GetTimeCurrent(tv1);
		ret = ty_net_recv(server_sock, &shead, sizeof(TySock_head_t), g_conf.server_tmout);
		if ((u_int)ret != sizeof(TySock_head_t) || shead.length > max_read_size) {
			WARNING_LOG("recv head to from[%d_%d] failed [%d/%u/%u]",
				group_id, loginfo.server_id, ret, sizeof(TySock_head_t), max_read_size);
			server_err_record(group_id, loginfo.server_id, server_sock, &gs);
			loginfo.server_status = SRV_GET_ERROR;
			loginfo.status = SRV_GET_ERROR;
			loginfo.server_timeused = 0;
			shead.status = SRV_GET_ERROR;
			goto send_to_apache;
		}
		ret = ty_net_recv(server_sock, read_buf, shead.length, g_conf.server_tmout);
		if ((u_int)ret != shead.length) {
			WARNING_LOG("recv body to from[%d_%d] failed [%d/%u]",
				group_id, loginfo.server_id, ret, shead.length);
			server_err_record(group_id, loginfo.server_id, server_sock, &gs);
			loginfo.server_status = SRV_GET_ERROR;
			loginfo.status = SRV_GET_ERROR;
			loginfo.server_timeused = 0;
			shead.status = SRV_GET_ERROR;
			goto send_to_apache;
		}

		GetTimeCurrent(tv2);
		SetTimeUsed(loginfo.server_timeused, tv1, tv2);

		shead.status = OK;
		loginfo.status = OK;
		goto send_to_apache;
merge_result:

		if (g_conf.serv_type != SERV_TYPE_SEARCH) {
			WARNING_LOG("recv req type err, not SERV_TYPE_SEARCH");
			loginfo.status = APACHE_REQ_TYPE_ERROR;
			goto message_end;
		}

		if (shead.need_pb == 1) {
			loginfo.tn_index = temp_get_template_index(as_apache_request.tn);//替换apache_request by luodongshan 2011/05/11
			if (loginfo.tn_index < 0) {
				WARNING_LOG("notn[%s] ip[%u]", as_apache_request.tn, shead.ip);//替换apache_request by luodongshan 2011/05/11
				loginfo.status = APACHE_NOTN_ERROR;
				goto message_end;
			}
		}

		if (shead.need_merge == 1) {
			// merge the reselt;
			loginfo.need_merge = 1;
			GetTimeCurrent(tv1);
			ret = as_server_merge_result(pmemhead, pmemnode, read_buf, max_read_size,
						&gs, &upstream_response, &upstream_request, upstream_result, MAX_SEARCH_RES_NUM,
						&loginfo, &as_apache_request);
			if(ret != 0 && ret != OK_FROM_CACHE) {
				WARNING_LOG("as_server_merge_result failed ret:%d", ret);
			}
			GetTimeCurrent(tv2);
			SetTimeUsed(loginfo.digest_server_timeused, tv1, tv2);
			loginfo.merge_status = ret;
			if(loginfo.merge_status < 0) {
				loginfo.merge_status = UI_OTHER_ERROR;
			}
		}

		if (shead.need_pb == 1) {
			loginfo.need_pb = 1;
			if (shead.need_merge == 0) {
				memcpy(&upstream_response, read_buf, sizeof(upstream_response_head_t));
				memcpy(upstream_result, read_buf + sizeof(upstream_response_head_t), sizeof(result_node_t) * upstream_response.return_num);
			}
			snprintf(loginfo.tn, sizeof(loginfo.tn), "%s", as_apache_request.tn);
			iserr = ((loginfo.merge_status == DISABLE_WORD_ERROR) ? 1 : 0);

			ret = as_temp_make_page(send_buf, max_send_size, &upstream_response,
								&as_apache_request, &upstream_request, upstream_result,
								&loginfo, &re_res, iserr);
//			printf("as_temp_make_page return:%d length:%d num:%d\n", ret, shead.length, upstream_response.return_num);
			if (ret < 0) {
				as_temp_make_page(send_buf, max_send_size, &upstream_response,
								&as_apache_request, &upstream_request, upstream_result,
								&loginfo, &re_res, 1);
				loginfo.status = UI_OTHER_ERROR;
				shead.length = strlen(send_buf);
			} else {
				shead.length = ret;
			}
//			printf("as_temp_make_page return:%d length:%d\n", ret, shead.length);

		}
//		shead.status = OK;
//		loginfo.status = OK;
		shead.status = loginfo.status;
		if(loginfo.merge_status > 0) {
			shead.status = loginfo.merge_status;
		}

send_to_apache:
		DEBUG_LOG("before send apache head");
		ret = ty_net_send(sock, &shead, sizeof(TySock_head_t));
		if ((u_int)ret != sizeof(TySock_head_t)) {
			WARNING_LOG("send head to apache failed [%d/%u]", ret, sizeof(TySock_head_t));
			loginfo.status = APACHE_PUT_ERROR;
			goto message_end;
		}
		DEBUG_LOG("before send apache body");

		if (shead.need_pb == 1 || shead.need_merge == 1) {
			ret = ty_net_send(sock, send_buf, shead.length);
		} else {
			ret = ty_net_send(sock, read_buf, shead.length);
		}

		if ((u_int)ret != shead.length) {
			WARNING_LOG("send body to apache failed [%d/%u]", ret, shead.length);
			loginfo.status = APACHE_PUT_ERROR;
		}
		DEBUG_LOG("end send apache body");

message_end:
		GetTimeCurrent(tvend);
		SetTimeUsed(timeused, tvstart, tvend);
		//status[%d][%d] query[%s] pid[%s] total[%u] ip[%u] srv[%u] dig[%u] time[%u]

		INFO_LOG("status[%d][%d] query[%s] pid[%s] total[%u] ip[%u] srv[%u] merge[%u] time[%u]nd[%d]na[%d]ns[%d]ji[%d][tm:%s]",
			loginfo.status, loginfo.merge_status, upstream_request.query, as_apache_request.pid_str,
			upstream_response.total_num, loginfo.ip,
			loginfo.server_timeused, loginfo.digest_server_timeused, timeused,
			upstream_request.need_del, upstream_request.must_and,
			upstream_request.need_synonym, upstream_request.result_just_int,
			as_apache_request.tn);
	}
}
*/

int worker_handler(conn_ctx_t *ctx) {

  empty_response_buf(&ctx->response_buf);
  char *buf = ctx->response_buf.base;
  int len = 0;

  memcpy(buf, &ctx->upstream_response, ctx->upstream_response_len);
  len = ctx->upstream_response_len;

  ctx->response_buf.len = len;
  return 1;
}
