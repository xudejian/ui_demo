#ifndef __UI_SERVER_H_
#define __UI_SERVER_H_

#include <uv.h>
#include "define.h"

#include "interface.h"
#include "search.h"

#define MAX_SERVER_HOST_LEN 32
#define GROUP_MAX_SERVER_NUM 10
#define MAX_GROUP_NUM 16
#define DEFAULT_GROUP_RATE 0

#define MAX_REFERER_ID 10*1000
typedef struct tag_Obj_server_t
{
	char host[MAX_SERVER_HOST_LEN];
	int port;
}Obj_server_t;

typedef struct tag_Server_group_t
{
	Obj_server_t servers[GROUP_MAX_SERVER_NUM];
	int srv_num;
	int rate;                               // 分流比例
}Server_group_t;

typedef enum {
	upstream_connect_init = 0,
	upstream_connecting,
	upstream_idle,
	upstream_work,
	upstream_close,
	upstream_connect_fail
} upstream_connect_status_t;

typedef struct {
	uv_connect_t connect;
	uv_tcp_t tcp;
	uv_write_t write;
	uv_timer_t timer;
	int status;
	int group_idx;
	int server_idx;
	u_int deadtime;     // 上次连接失败时间
	int need_close;
} upstream_connect_t;

typedef struct tag_Group_server_sock_t
{
	upstream_connect_t upstream[GROUP_MAX_SERVER_NUM];
	int real_num;
	int sock_num;
	int connecting;
	int lru;
}Group_server_sock_t;

typedef struct tag_Group_sock_t
{
	Group_server_sock_t group[MAX_GROUP_NUM];
	int distribute_count;
	int response_count;
}Group_sock_t;

typedef struct tag_Group_rate_t
{
	int rate[MAX_GROUP_NUM];
	int id[MAX_GROUP_NUM];
	int rate_num;

	int nid[MAX_GROUP_NUM];
	int nrate_num;
}Group_rate_t;

typedef struct tag_Referer_group_t
{
	char referer_name[WORD_SIZE];
	int group_id;
}Referer_group_t;

typedef struct {
	int magic;
	char query[MAX_QUERY_WORD_LEN];	//  q= 检索关键词
	char need_merge;
	char need_pb;
} web_request_t;

#define REQUEST_BUF_SIZE sizeof(web_request_t)
#define RESPONSE_BUF_SIZE 65535

typedef struct {
	int magic;
	char  query[MAX_QUERY_WORD_LEN];
	u_short page_no;
	u_short rn;
	u_short sort;
	u_short other;
	char  tn[MAX_TEMPLATE_NAME_LEN];
	char need_merge;
	char need_pb;
} upstream_request_t;

typedef struct {
	u_int type;
	u_int width;
	u_int height;
	u_int capacity;
	u_int tpl;
} upstream_response_head_data_t;

typedef struct {
	u_int status;
	u_int return_num;					//返回结果条数
	u_int res_num;						//返回结果条数
	u_int total_num;					//搜索到结果总数
	u_int len;
	upstream_response_head_data_t data;
} upstream_response_head_t;

typedef struct {
  uv_work_t worker;
  uv_write_t write;
  uv_stream_t client;

  uv_buf_t response_buf;

  int need_close;
  int status;
  void *data;
  Group_sock_t gs;

  struct {
	  u_int len;
	  upstream_response_head_t head;
	  char buf[RESPONSE_BUF_SIZE];
	  char padding[RESPONSE_BUF_SIZE];
  } upstream_response;

  int request_len;
  struct {
    web_request_t web;
	upstream_request_t up;
  } request;

  union {
    char buf[RESPONSE_BUF_SIZE];
  } _response;
} conn_ctx_t;

extern uv_tcp_t * server_listen(const char *ip, int port, uv_loop_t *loop);

// old
extern int server_conf_load(char *confpath);

extern int server_get_group_id(int refererid);

extern int server_connect_all(Group_sock_t *pgs);

extern int server_get_sock(int *group_id, int *server_id, Group_sock_t *pgs);

extern int server_err_record(int group_id, int server_id, int sock, Group_sock_t *pgs);

extern int as_server_merge_result(Mem_handle_t *pmemhead, Mem_handle_t *pmemnode,char *read_buf, u_int buf_size,
						Group_sock_t *pgs, upstream_response_head_t *preshead, upstream_request_t *preq, result_node_t *presnode, int res_num,
						Session_t *loginfo, web_request_t *p_web_req);

extern int group_result_by_pid(Search_result_t *pres, int res_num, u_int *ptop_pid);


#endif
