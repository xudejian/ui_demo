#ifndef __SERVER_DEFINE_H_
#define __SERVER_DEFINE_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

#include "zwlog.h"
#include "util.h"

#define PROJECT_NAME 	"ui"
#define SERVER_VERSION  "0.0.1"

#define DEFAULT_THREAD_NUM 4
#define DEFAULT_IP_TYPE 1
#define DEFAULT_TOP_PID_NUM 10

#define DEFAULT_ENABLE_MERGE_STAT false
#define DEFAULT_ENABLE_UNIQUE_RESULT true
#define DEFAULT_ENABLE_DIGEST false
#define DEFAULT_DIGEST_TYPE 0
#define DEFAULT_ENABLE_PID_REL false
#define DEFAULT_ENABLE_INDEX_TOP false

#define DEFAULT_ENABLE_UI_CACHE true

#define DEFAULT_CONF_TITLE_LEN 0

#define SERV_TYPE_SEARCH 1
#define SERV_TYPE_FILTER 2

//需要返回删除的文档
#define NEED_DEL_DOC true
//不需要返回已经删除的文档
#define NOT_NEED_DEL_DOC false
//默认为不需要从引擎返回已经删除的文档
#define DEFAULT_NEED_DOC_TYPE NOT_NEED_DEL_DOC


//完全与的归并方式
#define MUST_AND_DOC true
//不完全与的归并方式
#define NOT_MUST_AND_DOC false
//默认为不采用完全与的归并方式
#define DEFAULT_MUST_AND_TYPE NOT_MUST_AND_DOC
//默认的高亮字符串的最大长度
#define DEFAULT_HIGH_LITHG_MAX_LEN 256


//支持同义词
#define NEED_SYNONYM_DOC true
//不支持同义词
#define NOT_NEED_SYNONYM_DOC false
//默认为支持同义词
#define DEFAULT_NEED_SYNONYM_TYPE NEED_SYNONYM_DOC



#define SERVER_RECONNECT_TIME 300

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 256
#endif

#ifndef WORD_SIZE
#define WORD_SIZE 32
#endif

#define DEFAULT_LOG_PATH "../logs"
#define DEFAULT_LOG_EVENT 0xff
#define DEFAULT_LOG_OTHER 0xff
#define DEFAULT_LOG_SIZE 64000000

#define MAX_PID_NUM	2048
#define MAX_TEMPLATE_NUM	32
#define MAX_TEMPLATE_NAME_LEN	32

#define GetTimeCurrent(tv) gettimeofday(&tv, NULL)
#define SetTimeUsed(tused, tv1, tv2) { \
	tused  = (tv2.tv_sec-tv1.tv_sec) * 1000; \
	tused += (tv2.tv_usec-tv1.tv_usec) / 1000; \
	if (tused == 0){ tused+=1; } \
}

#ifndef FATAL_LOG
#define FATAL_LOG(fmt, ...) printf(RED"%s:%d (%s): " fmt "\n"NONE, __FILE__, __LINE__, __func__, ## __VA_ARGS__)
#endif

#ifndef u_int
#define uint32_t u_int
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 1024
#endif

#ifdef DEBUG
#define EOL "\n"
#else
#define EOL
#endif

#ifndef container_of
#define container_of(ptr, type, field)                                        \
  ((type *) ((char *) (ptr) - offsetof(type, field)))
#endif

typedef struct {
	int status;
	int merge_status;
	u_int ip;
	int group_id;
	int server_id;
	int fetch_timeused;
	int digest_server_timeused;
	int re_timeused;
	int server_timeused;
	int server_status;
	u_int need_merge;
	u_int need_pb;
	char tn[MAX_TEMPLATE_NAME_LEN];
	int tn_index;
	int cache_offset;
}Session_t;

typedef struct {} Mem_handle_t;

#define DEFAULT_SERVER_TIMEOUT -1

#endif
