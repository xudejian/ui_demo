#ifndef MAJIANG_LOG_H_
#define MAJIANG_LOG_H_

#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[0;32;32m"
#define LIGHT_GREEN "\033[1;32m"
#define BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"
#define LIGHT_GRAY "\033[0;37m"
#define WHITE "\033[1;37m"

#define WARNING_LOG(fmt, ...) printf(RED"%s:%d (%s): " fmt "\n"NONE, __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define DEBUG_LOG(fmt, ...) printf(GREEN"%s:%d (%s): " fmt "\n"NONE, __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define INFO_LOG(fmt, ...) printf(BLUE"" fmt "\n"NONE,  ## __VA_ARGS__)

#define LOG(fmt, ...) printf(WHITE"" fmt "\n"NONE, ## __VA_ARGS__)


#endif // MAJIANG_LOG_H_
