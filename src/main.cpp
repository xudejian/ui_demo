#include <time.h>
#include <uv.h>

#include "conf.h"
#include "server.h"
#include "core.h"
#include "template.h"


Conf_t g_conf;
char g_sys_path[MAX_PATH_LEN];

void usage(char * prname)
{
	printf("\n");
	printf("Project    : %s\n", PROJECT_NAME);
	printf("Version    : %s\n", SERVER_VERSION);
	printf("Usage: %s [-v]\n", prname);
	printf("	[-v]  show version info\n");
	printf("\n");
}

int main_parse_option(int argc, char **argv)
{
	if(argc != 1)
	{
		usage(argv[0]);
		exit(1);
	}

	char *mainpath = getenv("UI_PATH");
	if (mainpath == NULL)
		snprintf(g_sys_path, sizeof(g_sys_path), "../");
	else
		snprintf(g_sys_path, sizeof(g_sys_path), "%s", mainpath);
	return 1;
}

int init()
{
	signal(SIGPIPE, SIG_IGN);
	srand(time(NULL));
	// load_sysconf
	char temp_path[MAX_PATH_LEN];
	snprintf(temp_path, sizeof(temp_path), "%s/conf/ui.conf", g_sys_path);
	if (load_conf(&g_conf, temp_path) < 0) {
		return -1;
	}

	snprintf(temp_path, sizeof(temp_path), "%s/conf", g_sys_path);
	if (server_conf_load(temp_path) < 0) {
		return -1;
	}

	if (template_init() < 0) {
		FATAL_LOG("call template_init() failed!");
		return -1;
	}
	snprintf(temp_path, sizeof(temp_path), "%s/templates", g_sys_path);
	if(template_load(temp_path) < 0) {
		FATAL_LOG("template_load() failed!");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	main_parse_option(argc, argv);
	if (init() < 0) {
		FATAL_LOG("fail to init!");
		return EXIT_FAILURE;
	}

	uv_loop_t *loop = uv_loop_new();
	const char *ip = "0.0.0.0";
	const int port = g_conf.listen_port;

	uv_tcp_t *server = server_listen(ip, port, loop);
	if (!server) {
		DEBUG_LOG("Listen error %s", uv_err_name(uv_last_error(loop)));
		return EXIT_FAILURE;
	}
	DEBUG_LOG("listening %s:%d", ip, port);
	uv_run(loop, UV_RUN_DEFAULT);
	return EXIT_SUCCESS;
}
