#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "server.h"
#include "core.h"
#include "conf.h"

static Server_group_t g_sgroup[MAX_GROUP_NUM];
static int g_sg_num;//分组的数量
static Referer_group_t g_rg[MAX_REFERER_ID];
static Group_rate_t g_rate;

extern Conf_t g_conf;

int server_get_group_id(int refererid)
{
	if (refererid >= MAX_REFERER_ID || refererid < 0)
		return -1;
	return g_rg[refererid].group_id;
}

/*
int server_connect_all(Group_sock_t *pgs)
{
	if (pgs == NULL)
		return -1;
	for (int i = 0; i < g_sg_num; i++)
	{
		pgs->group[i].real_num = g_sgroup[i].srv_num;
		pgs->group[i].sock_num = 0;
		for (int sn = 0; sn < pgs->group[i].real_num; sn++)
		{
			pgs->group[i].sock[sn] = ty_tcpconnect(g_sgroup[i].servers[sn].host, g_sgroup[i].servers[sn].port, 1);
			if (pgs->group[i].sock[sn] < 0)
			{
				WARNING_LOG("connect host[%s] port[%d] failed!",
					g_sgroup[i].servers[sn].host, g_sgroup[i].servers[sn].port);
				pgs->group[i].deadtime[sn] = (u_int)time(NULL);
			}
			else
			{
				pgs->group[i].sock_num++;
			}
		}
		if (pgs->group[i].sock_num == 0)
		{
			WARNING_LOG("connect group[%d] failed!", i);
		}
	}
	return 0;
}
*/

int server_err_record(int group_id,int server_id,int sock,Group_sock_t * pgs)
{
	int gid = group_id - 1;
	int sid = server_id - 1;
	close(sock);

	pgs->group[gid].deadtime[sid] = (u_int)time(NULL);
	pgs->group[gid].sock[sid] = -1;
	pgs->group[gid].sock_num--;
	WARNING_LOG("host[%s] port[%d] closed and retry %d second later!",
		g_sgroup[gid].servers[sid].host, g_sgroup[gid].servers[sid].port, SERVER_RECONNECT_TIME);

	return 0;
}

/*
int server_get_sock(int *group_id, int *server_id, Group_sock_t *pgs)
{
	int gid = *group_id;

	if (gid == 0)   // rand the group with rate
	{
		// get a rand num from 1 to 100
		int rnum = (int) (100.0*rand()/(RAND_MAX+1.0));
		int i = 0;
		for (i = 0; i < g_rate.rate_num; i++)
		{
			if (rnum < g_rate.rate[i])
				break;
		}
		if (i < g_rate.rate_num)
		{
			gid = g_rate.id[i];
		}
		else
		{
			// rand from other group
			float range = (float)g_rate.nrate_num;
			rnum = (int)(range * rand()/(RAND_MAX+1.0));
			gid = g_rate.nid[rnum];
		}
	}

	// rand server id in group
	*group_id = gid;
	gid--;

	if (pgs->group[gid].real_num < 1) {
		*server_id = -1;
		return -1;
	}

	time_t tnow;
	time(&tnow);
	// all the sock is error, open all to test
	if (pgs->group[gid].sock_num == 0)
	{
		for (int sn = 0; sn < pgs->group[gid].real_num; sn++)
		{
			pgs->group[gid].sock[sn] = ty_tcpconnect(g_sgroup[gid].servers[sn].host, g_sgroup[gid].servers[sn].port, 1);
			pgs->group[gid].deadtime[sn] = tnow;
			if (pgs->group[gid].sock[sn] < 0)
			{
				WARNING_LOG("connect host[%s] port[%d] failed!",
					g_sgroup[gid].servers[sn].host, g_sgroup[gid].servers[sn].port);
			}
			else
			{
				pgs->group[gid].sock_num++;
			}
		}
		if (pgs->group[gid].sock_num == 0)
		{
			WARNING_LOG("connect group[%d] failed!", gid);
			*server_id = -1;
			return -1;
		}
	}

	// all the sock also error return -1
	if (pgs->group[gid].sock_num == 0)
	{
		*server_id = -1;
		WARNING_LOG("group[%d] no more socket!", gid);
		return -1;
	}

	int snum = tnow % pgs->group[gid].real_num;
	int try_time = 0;
	do {
		while (pgs->group[gid].sock[snum] < 0)
		{
			if (tnow - pgs->group[gid].deadtime[snum] > SERVER_RECONNECT_TIME)
			{
				pgs->group[gid].sock[snum] = ty_tcpconnect(g_sgroup[gid].servers[snum].host, g_sgroup[gid].servers[snum].port, 1);
				pgs->group[gid].deadtime[snum] = tnow;
				if (pgs->group[gid].sock[snum] < 0)
				{
					WARNING_LOG("retry host[%s] port[%d] failed!",
							g_sgroup[gid].servers[snum].host, g_sgroup[gid].servers[snum].port);
				}
				else
				{
					pgs->group[gid].sock_num++;
					*server_id = (snum+1);
					return pgs->group[gid].sock[snum];
				}
			}
			++try_time;
			snum = (snum + 1) % pgs->group[gid].real_num;
			if (try_time > pgs->group[gid].real_num) {
				*server_id = -1;
				return -1;
			}
		}
		*server_id = (snum+1);

		if (tnow - pgs->group[gid].deadtime[snum] > SERVER_RECONNECT_TIME) {
			pgs->group[gid].deadtime[snum] = tnow;
			if (is_socket_epipe(pgs->group[gid].sock[snum])) {
				server_err_record(*group_id, *server_id, pgs->group[gid].sock[snum], pgs);
				pgs->group[gid].deadtime[snum] = tnow - SERVER_RECONNECT_TIME - 1;
				continue;
			}
		}
		return pgs->group[gid].sock[snum];
	} while (1);
	*server_id = -1;
	return -1;
}
*/

static void server_conf_init()
{
	g_sg_num = 0;
	memset(&g_rate, 0, sizeof(Group_rate_t));
	memset(&g_sgroup, 0, sizeof(Server_group_t));
	memset(g_rg, 0, sizeof(g_rg));
}

#define CONF_INT(name, item)                 \
	if (!strncasecmp(key, name, key_len)) {  \
		item = atoi(value);					 \
		return;                              \
	}

#define CONF_STR(name, item)                                   \
	if (!strncasecmp(key, name, key_len)) {                    \
		memcpy(item, value, sizeof(item) - 1);                 \
		item[sizeof(item) - 1] = '\0';                         \
		return;                                                \
	}

static void server_assign_conf_item(void *data, char *key, int key_len, char *value)
{
	CONF_INT("Server_group_num", g_sg_num);
	if (is_start_with_end_with_s(key, key_len, "Group_", "_rate")) {
		int idx = atoi(key+6) - 1;
		if (idx >= g_sg_num || idx < 0)
		{
			WARNING_LOG("the idx [%d] is out of range", idx);
			return;
		}

		g_sgroup[idx].rate = atoi(value);
	}
}

static void server_group_assign_conf_item(void *data, char *key, int key_len, char *value)
{
	if (NULL == data) {
		return;
	}

	Server_group_t *conf = (Server_group_t *)data;
	CONF_INT("Server_num", conf->srv_num);
	if (conf->srv_num < 1) {
		return;
	}

	if (is_start_with_s(key, "Srv_")) {
		int idx = atoi(key+4);
		if (idx >= conf->srv_num || idx < 0)
		{
			WARNING_LOG("the idx [%d] is out of range", idx);
			return;
		}

		if (is_end_with_s(key, key_len, "_host")) {
			msetstr(conf->servers[idx].host, value);
		}
		else if (is_end_with_s(key, key_len, "_port")) {
			conf->servers[idx].port = atoi(value);
		}
	}
}

static void server_referer_group_assign_conf_item(void *data, char *key, int key_len, char *value)
{
	if (is_start_with_s(key, "Ref_")) {
		int r_id = atoi(key+4);
		if (r_id >= MAX_REFERER_ID || r_id < 0)
		{
			WARNING_LOG("the referer id [%d] is out of range", r_id);
			return;
		}

		if (is_end_with_s(key, key_len, "_name")) {
			msetstr(g_rg[r_id].referer_name, value);
		}
		else if (is_end_with_s(key, key_len, "_group")) {
			g_rg[r_id].group_id = atoi(value);
		}
	}
}
#undef CONF_INT
#undef CONF_STR

int server_conf_load(char *confpath)
{
	int rv;
	char filename[1024];

	server_conf_init();

	snprintf(filename, sizeof(filename)-1, "%s/server.conf", confpath);
	rv = async_load_conf(filename, NULL, server_assign_conf_item);
	if (rv < 0) {
		WARNING_LOG("read conf[%s] failed!", filename);
		return -1;
	}
	if (g_sg_num > MAX_GROUP_NUM)
		g_sg_num = MAX_GROUP_NUM;


	int rate = 0;
	for (int i = 0; i < g_sg_num; i++)
	{
		if (g_sgroup[i].rate > 0)
		{
			g_rate.rate[g_rate.rate_num] = rate + g_sgroup[i].rate;
			g_rate.id[g_rate.rate_num] = i+1;
			rate = g_rate.rate[g_rate.rate_num];
			g_rate.rate_num++;
		}
		else
		{
			g_rate.nid[g_rate.nrate_num] = i+1;
			g_rate.nrate_num++;
		}

	}

	// load group_num.conf
	for (int i = 0; i < g_sg_num; i++)
	{
		snprintf(filename, sizeof(filename)-1, "%s/group_%d.conf", confpath, i+1);
		rv = async_load_conf(filename, (void*)&g_sgroup[i], server_group_assign_conf_item);
		if (rv < 0) {
			WARNING_LOG("read conf[%s] failed!", filename);
			g_sgroup[i].srv_num = 0;
			continue;
		}

		if (g_sgroup[i].srv_num > GROUP_MAX_SERVER_NUM)
			g_sgroup[i].srv_num = GROUP_MAX_SERVER_NUM;
	}

	// load referer_group.conf
	snprintf(filename, sizeof(filename)-1, "%s/referer_group.conf", confpath);
	rv = async_load_conf(filename, NULL, server_referer_group_assign_conf_item);
	if (rv < 0) {
		WARNING_LOG("read conf[%s] failed!", filename);
		return 0;
	}

	return 0;
}

conn_ctx_t *alloc_conn_context() {
  conn_ctx_t *ctx = (conn_ctx_t *) malloc(sizeof(conn_ctx_t));
  ctx->data = NULL;
  ctx->request_len = 0;
  DEBUG_LOG("alloc conn context");
  return ctx;
}

uv_buf_t alloc_request_buf(uv_handle_t *client, size_t suggested_size) {
  conn_ctx_t *ctx = container_of(client, conn_ctx_t, client);
  DEBUG_LOG("in alloc_request_buf");
  return uv_buf_init((char *)&ctx->request.web, REQUEST_BUF_SIZE + RESPONSE_BUF_SIZE);
}

void context_clean_cb(uv_handle_t* client) {
  conn_ctx_t *ctx = container_of(client, conn_ctx_t, client);
  server_ctx_clean(ctx);
  free(ctx);
  DEBUG_LOG("free context");
}

static void on_request(uv_stream_t *client, ssize_t nread, uv_buf_t buf);

void response_send_cb(uv_write_t *write, int status) {
  conn_ctx_t *ctx = container_of(write, conn_ctx_t, write);
  if (status < 0) {
    uv_loop_t *loop = ctx->client.loop;
    WARNING_LOG("Write error %s", uv_err_name(uv_last_error(loop)));
  }
  DEBUG_LOG("response send done");
  uv_close((uv_handle_t*) &ctx->client, context_clean_cb);
}

void handle_request(uv_work_t *worker) {
  conn_ctx_t *ctx = container_of(worker, conn_ctx_t, worker);
  int rv = worker_handler(ctx);
  DEBUG_LOG("done in work %d", rv);
}

void send_response(uv_work_t *worker, int status) {
  uv_loop_t *loop = worker->loop;
  conn_ctx_t *ctx = container_of(worker, conn_ctx_t, worker);
  if (status < 0 && uv_last_error(loop).code == UV_ECANCELED) {
    return;
  }
  DEBUG_LOG("send response [%s]", ctx->response_buf.base);
  uv_write(&ctx->write, &ctx->client, &ctx->response_buf, 1, response_send_cb);
}

static void on_request(uv_stream_t *client, ssize_t nread, uv_buf_t buf) {
  if (nread == 0) {
    DEBUG_LOG("read 0");
    return;
  }

  conn_ctx_t *ctx = container_of(client, conn_ctx_t, client);
  uv_loop_t *loop = client->loop;
  if (nread < 0) {
    if (uv_last_error(loop).code != UV_EOF) {
      WARNING_LOG("Read error %s", uv_err_name(uv_last_error(loop)));
    }
    DEBUG_LOG("nread %ld < 0 and close client", nread);
    uv_cancel((uv_req_t*) &ctx->worker);
    uv_close((uv_handle_t*) client, context_clean_cb);
    return;
  }

  ctx->request_len += nread;
  if (ctx->request_len > (int)REQUEST_BUF_SIZE) {
    ctx->request_len = REQUEST_BUF_SIZE;
    return;
  }

  DEBUG_LOG("read %ld/%d/%ld", nread, ctx->request_len, REQUEST_BUF_SIZE);
  ctx->response_buf.base = ctx->_response.buf;
  ctx->response_buf.len = 0;
  if (ctx->request.web.magic != sizeof(web_request_t)) {
    WARNING_LOG("read request's magic is not correct");
    uv_close((uv_handle_t*) client, context_clean_cb);
    return;
  }

  uv_queue_work(loop, &ctx->worker, handle_request, send_response);
}

void on_new_connection(uv_stream_t *server, int status) {
  if (status < 0) {
    // error!
    return;
  }

  uv_loop_t *loop = server->loop;
  conn_ctx_t *ctx = alloc_conn_context();
  uv_tcp_init(loop, (uv_tcp_t*) &ctx->client);
  if (uv_accept(server, (uv_stream_t*) &ctx->client) == 0) {
    INFO_LOG("accept once");
    uv_read_start(&ctx->client, alloc_request_buf, on_request);
  } else {
    DEBUG_LOG("accept fail");
    uv_close((uv_handle_t*) &ctx->client, context_clean_cb);
  }
}

uv_tcp_t *server_listen(const char *ip, int port, uv_loop_t *loop) {
  uv_tcp_t *server = (uv_tcp_t *)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, server);
  struct sockaddr_in bind_addr = uv_ip4_addr(ip, port);
  uv_tcp_bind(server, bind_addr);
  int rv = uv_listen((uv_stream_t*) server, 128, on_new_connection);
  if (rv) {
    free(server);
    return NULL;
  }

  return server;
}
