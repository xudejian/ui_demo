#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conf.h"
#include "define.h"
#include "search.h"

char* trim(char *str)
{
	char *p = str;
	while (*p && *p == ' ') {
		p++;
	}
	str = p;

	while (*p && *p != '\n') {
		p++;
	}
	if (*p == '\n') {
		*p = '\0';
		p--;
	}
	if (*p == '\r') {
		*p = '\0';
		p--;
	}
	return str;
}

void init_conf(Conf_t *conf) {
	if (NULL == conf) {
		return;
	}
	conf->listen_port = 3000;
	conf->write_tmout = -1;
	conf->read_tmout = -1;
	conf->thread_count = DEFAULT_THREAD_NUM;
	conf->ip_list_type = DEFAULT_IP_TYPE;
	conf->enable_merge_stat = DEFAULT_ENABLE_MERGE_STAT;
	conf->top_pid_num = DEFAULT_TOP_PID_NUM;
	conf->enable_unique_result = DEFAULT_ENABLE_UNIQUE_RESULT;
	conf->need_del = DEFAULT_NEED_DOC_TYPE;
	conf->must_and = DEFAULT_MUST_AND_TYPE;
	conf->need_synonym = DEFAULT_NEED_SYNONYM_TYPE;
	conf->server_tmout = DEFAULT_SERVER_TIMEOUT;
	conf->enable_pid_rel = DEFAULT_ENABLE_PID_REL;

	snprintf(conf->log_path, sizeof(conf->log_path), "%s", DEFAULT_LOG_PATH);
	snprintf(conf->log_name, sizeof(conf->log_name), "%s", PROJECT_NAME);
	conf->log_event = DEFAULT_LOG_EVENT;
	conf->log_other = DEFAULT_LOG_OTHER;
	conf->log_size = DEFAULT_LOG_SIZE;
	conf->title_length = DEFAULT_CONF_TITLE_LEN;
	conf->high_light_length = DEFAULT_HIGH_LITHG_MAX_LEN;
	conf->search_sort_def = SEARCH_SORT_DEF;
	conf->enable_index_top = DEFAULT_ENABLE_INDEX_TOP;

	conf->magic_ui_num = 0;
	conf->magic_server_num = 0;
}

int assign_conf_item(Conf_t *conf, char *key, int key_len, char *value)
{
#define CONF_INT(name, item)                 \
	if (!strncasecmp(key, name, key_len)) {  \
		conf->item = atoi(value);            \
		return 0;                            \
	}

#define CONF_STR(name, item)                                   \
	if (!strncasecmp(key, name, key_len)) {                    \
		snprintf(conf->item, sizeof(conf->item), "%s", value); \
		return 0;                                              \
	}

	CONF_INT("Listen_port", listen_port);
	CONF_INT("Read_timeout", read_tmout);
	CONF_INT("Write_timeout", write_tmout);
	CONF_INT("Thread_num", thread_count);
	CONF_INT("Ip_list_type", ip_list_type);
	CONF_INT("Enable_merge_stat", enable_merge_stat);
	CONF_INT("Top_pid_num", top_pid_num);
	if (conf->top_pid_num < 1) {
		conf->enable_merge_stat = 0;
	}

	CONF_INT("Enable_unique_result", enable_unique_result);
	CONF_INT("Need_del", need_del);
	CONF_INT("Must_and", must_and);
	CONF_INT("Need_synonym", need_synonym);

	CONF_INT("Server_read_timeout", server_tmout);

	CONF_STR("Log_path", log_path);
	CONF_STR("Log_name", log_name);
	CONF_INT("Log_event", log_event);
	CONF_INT("Log_other", log_other);
	CONF_INT("Log_size", log_size);
	CONF_INT("Title_length", title_length);
	if (conf->title_length < 0) {
		conf->title_length = 0;
	}

	CONF_INT("High_light_length", high_light_length);
	if (conf->high_light_length < 0) {
		conf->high_light_length = 0;
	}

	CONF_INT("Def_search_sort", search_sort_def);
	if (conf->search_sort_def < SEARCH_SORT_MIN)
		conf->search_sort_def = SEARCH_SORT_DEF;
	if (conf->search_sort_def > SEARCH_SORT_MAX)
		conf->search_sort_def = SEARCH_SORT_DEF;

	CONF_INT("Enable_index_top", enable_index_top);
	CONF_INT("Magic_ui_num", magic_ui_num);
	CONF_INT("Magic_server_num", magic_server_num);
	CONF_INT("Serv_type", serv_type);
	return -1;
#undef CONF_INT
#undef CONF_STR
}

int parse_config_line(Conf_t *conf, char *line) {
	char *key, *value, *p;
	int key_len;

	p = key = line;
	while (*p && *p != ' ') {
		p++;
	}
	key_len = p - key;

	while (*p && *p == ' ') {
		p++;
	}
	if (*p != ':') {
		return -1;
	}
	p++;

	while (*p && *p == ' ') {
		p++;
	}
	value = p;

	return assign_conf_item(conf, key, key_len, value);
}

int load_conf(Conf_t *conf, const char *conf_filename)
{
	if (NULL == conf || NULL == conf_filename) {
		return -1;
	}

	init_conf(conf);
	FILE *fp = fopen(conf_filename, "r");
	if (fp == NULL) {
		return -1;
	}

	char buf[1024];
	while (fgets(buf, sizeof(buf), fp)) {
		parse_config_line(conf, trim(buf));
	}

	fclose(fp);
	fp = NULL;
	return 0;
}
