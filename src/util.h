#ifndef __UI_UTIL_H_
#define __UI_UTIL_H_

#include "define.h"
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

void trimNR(char *src);
char *strstrGBK(char *src,const char *dest);
char * strcasestrGBK(char *src,const char *dest);

int cstr_split(char *src, int slen, char sep, char **obj, int size);
void str2lower(char *s);
int fixstring(char *s, size_t size, int word_len);
int XMLEncode(const char *src, char *obj, int size);
int XMLDecode(const char *src, char *obj, int size);
void gbk_blank_to_en(char *s);
int
quote_escape(const char *src, const size_t len, size_t *esclen,
	char *obj, const size_t size, size_t *use_len);
int
no_escape(const char *src, const size_t len, size_t *esclen,
	char *obj, const size_t size, size_t *use_len);

int is_socket_alive(int socket, int timeout);
int is_socket_epipe(int socket);

int get_gbk_from_utf8(char *dst, char *src);

#ifdef __cplusplus
}
#endif

#endif
