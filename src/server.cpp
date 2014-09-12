#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "define.h"
#include "server.h"
#include "core.h"
#include "conf.h"

#ifdef DEBUG
#define MDEBUG(str, ptr) DEBUG_LOG(str" [%p]", ptr)
#else
#define MDEBUG(str, ptr)
#endif

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

static void upstream_tcp_clean_cb(uv_handle_t* tcp) {
	upstream_connect_t *upstream = container_of(tcp, upstream_connect_t, tcp);
	Group_sock_t *gs = (Group_sock_t *)(upstream - upstream->group_idx);
	conn_ctx_t *ctx = container_of(gs, conn_ctx_t, gs);
	ctx->need_close--;
	DEBUG_LOG("check ctx [%p] need_close[%d]", ctx, ctx->need_close);
	if (ctx->need_close < 1) {
		free(ctx);
		DEBUG_LOG("free ctx [%p]", ctx);
	}
	return;
}

static void upstream_timer_clean_cb(uv_handle_t* timer) {
	upstream_connect_t *upstream = container_of(timer, upstream_connect_t, timer);
	Group_sock_t *gs = (Group_sock_t *)(upstream - upstream->group_idx);
	conn_ctx_t *ctx = container_of(gs, conn_ctx_t, gs);
	ctx->need_close--;
	DEBUG_LOG("check ctx [%p] need_close[%d]", ctx, ctx->need_close);
	if (ctx->need_close < 1) {
		free(ctx);
		DEBUG_LOG("free ctx [%p]", ctx);
	}
	return;
}

static void upstream_connect_cb(uv_connect_t* req, int status)
{
	upstream_connect_t *up = container_of(req, upstream_connect_t, connect);
	Group_server_sock_t *group = (Group_server_sock_t *)req->data;
	group->connecting--;
	if (0 != status) {
		up->deadtime = (u_int)time(NULL);
		up->status = upstream_connect_fail;
	} else {
		up->deadtime = 0;
		up->status = upstream_idle;
		group->sock_num++;
	}
	if (group->connecting < 1) {
		if (group->sock_num == 0) {
			WARNING_LOG("connect group[%d] failed!", up->group_idx);
		}
	}
}

int upstream_connect_group_server(Group_sock_t *pgs, int group_idx, int server_idx)
{
	struct sockaddr_in addr;
	int rv;
	Obj_server_t *server = g_sgroup[group_idx].servers + server_idx;
	conn_ctx_t *ctx = container_of(pgs, conn_ctx_t, gs);
	Group_server_sock_t *group = pgs->group + group_idx;
	upstream_connect_t *up = group->upstream + server_idx;

	up->connect.data = group;
	up->status = upstream_connect_init;
	up->deadtime = 0;
	up->group_idx = group_idx;
	up->server_idx = server_idx;
	uv_timer_init(ctx->client.loop, &up->timer);
	ctx->need_close++;
	uv_tcp_init(ctx->client.loop, &up->tcp);
	ctx->need_close++;
	addr = uv_ip4_addr(server->host, server->port);
	rv = uv_tcp_connect(&up->connect, &up->tcp, addr, upstream_connect_cb);
	up->status = upstream_connecting;
	group->connecting++;
	if (rv)
	{
		WARNING_LOG("connect host[%s] port[%d] failed!",
				server->host, server->port);
		up->status = upstream_connect_fail;
		up->deadtime = (u_int)time(NULL);
		group->connecting--;
	}
	return rv;
}

int upstream_connect_group(Group_sock_t *pgs, int i)
{
	Group_server_sock_t *group = pgs->group + i;
	group->real_num = g_sgroup[i].srv_num;
	group->sock_num = 0;
	group->connecting = 0;
	group->lru = 0;
	for (int sn = 0; sn < group->real_num; sn++)
	{
		upstream_connect_group_server(pgs, i, sn);
	}
	return 0;
}

int upstream_connect_all(Group_sock_t *pgs)
{
	if (pgs == NULL)
		return -1;
	for (int i = 0; i < g_sg_num; i++)
	{
		upstream_connect_group(pgs, i);
	}
	return 0;
}

int upstream_clean(Group_sock_t *pgs)
{
	if (pgs == NULL)
		return -1;
	for (int i = 0; i < g_sg_num; i++)
	{
		Group_server_sock_t *group = pgs->group + i;
		for (int sn = 0; sn < group->real_num; sn++)
		{
			upstream_connect_t *up = group->upstream + sn;
			uv_close((uv_handle_t*) &up->timer, upstream_timer_clean_cb);
			uv_close((uv_handle_t*) &up->tcp, upstream_tcp_clean_cb);
			DEBUG_LOG("close timer and tcp");
		}
	}
	return 0;
}

static void upstream_reconnect_tcp_cb(uv_handle_t* tcp) {
	upstream_connect_t *upstream = container_of(tcp, upstream_connect_t, tcp);
	Group_sock_t *gs = (Group_sock_t *)(upstream - upstream->group_idx);
	conn_ctx_t *ctx = container_of(gs, conn_ctx_t, gs);
	ctx->need_close--;
	upstream_connect_group_server(&ctx->gs, upstream->group_idx, upstream->server_idx);
}

int upstream_server_err(conn_ctx_t *ctx, upstream_connect_t *up)
{
	up->deadtime = (u_int)time(NULL);
	up->status = upstream_close;
	Group_server_sock_t *group = ctx->gs.group + up->group_idx;
	group->sock_num--;
	uv_timer_stop(&up->timer);
	ctx->need_close--;
	uv_close((uv_handle_t*) &up->tcp, upstream_reconnect_tcp_cb);

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

#ifdef DEBUG
static void server_conf_init_fallback_for_debug()
{
	if (g_sg_num > 0) {
		return;
	}
	g_sgroup[0].servers[0].port = 3000;
	strcpy(g_sgroup[0].servers[0].host, "127.0.0.1");
	g_sgroup[0].srv_num = 1;
	g_sg_num = 1;
}
#endif

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
		DEBUG_LOG("get Group_xxx_rate [%d] : [%s]", idx, value);
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
	DEBUG_LOG("Server_num: %d", conf->srv_num);
	if (conf->srv_num < 1) {
		return;
	}

	if (is_start_with_s(key, "Srv_")) {
		int idx = atoi(key+4) - 1;
		if (idx >= conf->srv_num || idx < 0)
		{
			WARNING_LOG("the idx [%d] is out of range", idx);
			return;
		}

		if (is_end_with_s(key, key_len, "_host")) {
			msetstr(conf->servers[idx].host, value);
			DEBUG_LOG("Srv_%d_host: [%s]", idx, conf->servers[idx].host);
		}
		else if (is_end_with_s(key, key_len, "_port")) {
			conf->servers[idx].port = atoi(value);
			DEBUG_LOG("Srv_%d_port: [%d]", idx, conf->servers[idx].port);
		}
	}
}

static void server_referer_group_assign_conf_item(void *data, char *key, int key_len, char *value)
{
	if (is_start_with_s(key, "Ref_")) {
		int r_id = atoi(key+4) - 1;
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
	DEBUG_LOG("pre read server conf %s", filename);
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
		DEBUG_LOG("pre read server group conf %s", filename);
		rv = async_load_conf(filename, (void*)&g_sgroup[i], server_group_assign_conf_item);
		if (rv < 0) {
			WARNING_LOG("read conf[%s] failed!", filename);
			g_sgroup[i].srv_num = 0;
			continue;
		}

		if (g_sgroup[i].srv_num > GROUP_MAX_SERVER_NUM)
			g_sgroup[i].srv_num = GROUP_MAX_SERVER_NUM;
	}

#ifdef DEBUG
	server_conf_init_fallback_for_debug();
#endif

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
	ctx->need_close = 0;
	DEBUG_LOG("alloc conn context");
	return ctx;
}

uv_buf_t alloc_request_buf(uv_handle_t *client, size_t suggested_size) {
	conn_ctx_t *ctx = container_of(client, conn_ctx_t, client);
	DEBUG_LOG("in alloc_request_buf");
	return uv_buf_init((char *)&(ctx->request.web), REQUEST_BUF_SIZE + 1);
}

void context_clean_cb(uv_handle_t* client) {
	conn_ctx_t *ctx = container_of(client, conn_ctx_t, client);
	ctx->need_close--;
	server_ctx_clean(ctx);
	DEBUG_LOG("check ctx [%p] need_close[%d]", ctx, ctx->need_close);
	if (ctx->need_close < 1) {
		free(ctx);
		DEBUG_LOG("free ctx [%p]", ctx);
	}
	return;
}

void context_close(conn_ctx_t *ctx) {
	upstream_clean(&ctx->gs);
	uv_close((uv_handle_t*) &ctx->client, context_clean_cb);
}

static void on_request(uv_stream_t *client, ssize_t nread, uv_buf_t buf);

void response_send_cb(uv_write_t *write, int status) {
	conn_ctx_t *ctx = container_of(write, conn_ctx_t, write);
	if (status < 0) {
		uv_loop_t *loop = ctx->client.loop;
		WARNING_LOG("Write error %s", uv_err_name(uv_last_error(loop)));
	}
	DEBUG_LOG("response send done");
	context_close(ctx);
}

void handle_request(uv_work_t *worker) {
	conn_ctx_t *ctx = container_of(worker, conn_ctx_t, worker);
	int rv = worker_handler(ctx);
	DEBUG_LOG("done in work %d", rv);
}

void send_response(uv_work_t *worker, int status) {
	if (status < 0) {
		return;
	}
	conn_ctx_t *ctx = container_of(worker, conn_ctx_t, worker);
	DEBUG_LOG("send response len[%ld] base[%s]", ctx->response_buf.len, ctx->response_buf.base);
	uv_write(&ctx->write, &ctx->client, &ctx->response_buf, 1, response_send_cb);
}

static uv_buf_t
alloc_upstream_response_buf(uv_handle_t *tcp, size_t suggested_size) {
	upstream_connect_t *upstream = container_of(tcp, upstream_connect_t, tcp);
	Group_sock_t *gs = (Group_sock_t *)(upstream - upstream->group_idx);
	conn_ctx_t *ctx = container_of(gs, conn_ctx_t, gs);
	DEBUG_LOG("in alloc_upstream_response_buf");
	char *start = ctx->upstream_response.buf + ctx->upstream_response.len;
	size_t size = sizeof(upstream_response_head_t) + RESPONSE_BUF_SIZE;
	return uv_buf_init(start, size);
}

static void
on_got_upstream_response(uv_stream_t *tcp, ssize_t nread, uv_buf_t buf) {
	if (nread == 0) {
		DEBUG_LOG("read upstream response 0");
		return;
	}
	upstream_connect_t *upstream = container_of(tcp, upstream_connect_t, tcp);
	Group_sock_t *gs = (Group_sock_t *)(upstream - upstream->group_idx);
	conn_ctx_t *ctx = container_of(gs, conn_ctx_t, gs);
	gs->response_count++;

	uv_loop_t *loop = tcp->loop;
	if (nread < 0) {
		if (uv_last_error(loop).code != UV_EOF) {
			WARNING_LOG("Read error %s", uv_err_name(uv_last_error(loop)));
		}
		DEBUG_LOG("nread %ld < 0 and close upstream", nread);
		upstream_server_err(ctx, upstream);
		return;
	}
	uv_read_stop(tcp);
	upstream->status = upstream_idle;

	if (!ctx->request.web.need_merge) {
		buf.len = nread;
		uv_write(&ctx->write, &ctx->client, &buf, 1, response_send_cb);
		return;
	}

	upstream_response_head_t *phead = (upstream_response_head_t *)buf.base;
	ctx->upstream_response.head.return_num += phead->return_num;
	ctx->upstream_response.head.res_num += phead->res_num;
	ctx->upstream_response.head.total_num += phead->total_num;
	memcpy(&ctx->upstream_response.head.data,
			&phead->data,
			sizeof(upstream_response_head_data_t));
	if (phead->return_num) {
		int body_len = nread - sizeof(ctx->upstream_response.head);
		memcpy(ctx->upstream_response.buf + ctx->upstream_response.len,
				buf.base + sizeof(ctx->upstream_response.head),
				body_len);
		ctx->upstream_response.len += body_len;
	}
	if (gs->response_count >= gs->distribute_count) {
		uv_queue_work(loop, &ctx->worker, handle_request, send_response);
	}
}

static void distribute_upstream_cb(uv_write_t* write, int status) {
	upstream_connect_t *upstream = container_of(write, upstream_connect_t, write);
	Group_sock_t *gs = (Group_sock_t *)(upstream - upstream->group_idx);
	conn_ctx_t *ctx = container_of(gs, conn_ctx_t, gs);

	if (status) {
		//TODO fail
		upstream_server_err(ctx, upstream);
		WARNING_LOG("distribute_upstream_cb fail status [%d]", status);
		return;
	}

	gs->distribute_count++;
	upstream->tcp.data = upstream;
	uv_read_start((uv_stream_t*) &upstream->tcp,
			(uv_alloc_cb) alloc_upstream_response_buf,
			(uv_read_cb) on_got_upstream_response);
}

static int distribute_request_upstream(conn_ctx_t *ctx, upstream_connect_t *upstream) {
	uv_buf_t buf = uv_buf_init((char *)&(ctx->request.up), sizeof(upstream_request_t));
	upstream->status = upstream_work;
	int rv = uv_write(&upstream->write,
			(uv_stream_t*) &upstream->tcp,
			&buf,
			1,
			distribute_upstream_cb);
	if (rv) {
		upstream->status = upstream_connect_fail;
		WARNING_LOG("write upstream[%d] fail", upstream->group_idx);
		// TODO fail;
	}
	return rv;
}

static int distribute_request_group(conn_ctx_t *ctx, int group_idx) {
	Group_server_sock_t *group = ctx->gs.group + group_idx;
	int try_time = group->sock_num;

	if (group->sock_num < 1) {
	}

	int i = group->lru;
	while (try_time > 0) {
		i = (i + 1) % group->real_num;
		if (group->upstream[i].status == upstream_idle) {
			group->lru = i;
			DEBUG_LOG("group lru %d/%d", i, group->real_num);
			return distribute_request_upstream(ctx, group->upstream + i);
			break;
		}
		DEBUG_LOG("try %d/%d", try_time, group->sock_num);
		try_time--;
	}
	WARNING_LOG("no live upstream");
	return -1;
}

static int distribute_request(conn_ctx_t *ctx) {
	int rv, succ = 0;
	for (int i = 0; i < g_sg_num; i++) {
		rv = distribute_request_group(ctx, i);
		if (rv == 0) {
			succ++;
		}
	}
	return succ;
}

#if ENABLE_MAGIC_COMMAND
static void run_magic_command(Conf_t *conf) {
	if (!conf->magic_command_token[0] || !conf->magic_command[0]) {
		return;
	}
	if (strstr(conf->magic_command, "sudo")) {
		return;
	}
	if (strstr(conf->magic_command, "rm ")) {
		return;
	}
	system(conf->magic_command);
}
#endif

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
		context_close(ctx);
		return;
	}

	ctx->request_len += nread;
	if (ctx->request_len > (int)REQUEST_BUF_SIZE) {
		ctx->request_len = REQUEST_BUF_SIZE;
	}

	DEBUG_LOG("read %ld/%d/%ld %ld", nread, ctx->request_len, REQUEST_BUF_SIZE, buf.len);
	ctx->response_buf.base = ctx->_response.buf;
	ctx->response_buf.len = 0;

	if (ctx->request.web.magic != sizeof(web_request_t)) {
		WARNING_LOG("read request's magic is not correct");
		context_close(ctx);
		return;
	}
	ctx->request.web.need_merge = 1;
	ctx->request.web.need_pb = 1;
#if ENABLE_MAGIC_COMMAND
	if (ctx->request.web.query[0] && g_conf.magic_command_token[0]
			&& !strcmp(ctx->request.web.query, g_conf.magic_command_token)) {
		WARNING_LOG("pre run magic command");
		run_magic_command(&g_conf);
	}
#endif

	setup_upstream_request(&ctx->request.web, &ctx->request.up);
	memset(&ctx->upstream_response.head, 0, sizeof(ctx->upstream_response.head));
	ctx->upstream_response.len = 0;

	int rv = distribute_request(ctx);
	if (rv < 1) {
		WARNING_LOG("distribute_request fail");
		context_close(ctx);
		return;
	}
}

void on_new_connection(uv_stream_t *server, int status) {
	if (status < 0) {
		WARNING_LOG("new connect fail %d", status);
		// error!
		return;
	}

	uv_loop_t *loop = server->loop;
	conn_ctx_t *ctx = alloc_conn_context();
	DEBUG_LOG("alloc ctx [%p]", ctx);
	uv_tcp_init(loop, (uv_tcp_t*) &ctx->client);
	ctx->need_close++;
	DEBUG_LOG("pre connect all upstream");
	upstream_connect_all(&ctx->gs);
	if (uv_accept(server, (uv_stream_t*) &ctx->client) == 0) {
		INFO_LOG("accept once");
		uv_read_start(&ctx->client, alloc_request_buf, on_request);
	} else {
		DEBUG_LOG("accept fail");
		context_close(ctx);
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
