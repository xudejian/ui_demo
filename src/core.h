#ifndef __SERVER_CORE_H_
#define __SERVER_CORE_H_

#include <signal.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "define.h"
#include "server.h"

#define OK  				200
#define OK_FROM_CACHE		201

#define APACHE_GET_ERROR  	301
#define APACHE_PUT_ERROR   	302
#define APACHE_IP_ERROR     303
#define APACHE_REFERER_ERROR 304
#define APACHE_MAGIC_ERROR   305
#define APACHE_REQ_TYPE_ERROR 306
#define APACHE_NOTN_ERROR 307

#define SRV_CONN_ERROR     	401
#define SRV_PUT_ERROR		402
#define SRV_GET_ERROR       403

#define DIG_SRV_CONN_ERROR  404
#define DIG_SRV_PUT_ERROR	405
#define DIG_SRV_GET_ERROR   406

#define UI_OTHER_ERROR	   	501

#define DISABLE_WORD_ERROR  601

#define IS_GOOD_STATUS(status) ((status)/100 == 2)
#define IS_NEED_CLOSE_CLIENT(status) ((status)/100 == 3)


int server_init();
int worker_handler(conn_ctx_t *ctx);
void server_ctx_clean(conn_ctx_t *ctx);
int setup_upstream_request(web_request_t *web, upstream_request_t *up);

#endif
