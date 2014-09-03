#ifndef __SERVER_CONF_H_
#define __SERVER_CONF_H_

#include "define.h"

typedef struct {
	// setenv UV_THREADPOOL_SIZE
	int thread_count;              //模块线程数

	// socket
	int listen_port;                 //侦听端口
	int read_tmout;
	int write_tmout;

	// log file
	char log_path[MAX_PATH_LEN];
	char log_name[MAX_PATH_LEN];
	int log_event;
	int log_other;
	int log_size;

	// other
	int ip_list_type;                // 1 白名单 2 黑名单

	bool enable_merge_stat;          //是否开启聚合统计
	int top_pid_num;                 //pid聚合数量
	bool enable_unique_result;       //是否开启结果排重
	bool enable_digest;              //是否开启摘要
	int digest_type;                 //摘要类型 0静态 1动态

	bool enable_pid_rel;            //开启相关版块
	bool enable_ui_cache;			//是否开户ui的缓存，默认开户

	int title_length;              //标题长度
	int high_light_length;         //高亮字符串的最大长度

	int cache_size;                 //缓存大小
	int server_tmout;               //server read timeout
	int digest_server_tmout;        //digest server read timeout
	int digest_write_tmout;        //digest server write timeout

	int search_sort_def;

	bool enable_index_top;
	bool enable_re;
	int enable_proto_buf;//开启protobuf数据格式与引擎交互
	int proto_buf_rate;//使用protobuf协议的数据比例

	int magic_ui_num;
	int magic_server_num;
	int serv_type;

	int need_del;
	int	must_and;			// 是否完全与的归并方式
	int	need_synonym;		// 是否需要支持同义词

} Conf_t;

int load_conf(Conf_t *conf, const char *conf_filename);

#endif
