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

int get_gbk_from_utf8(char *dst, char *src);

#define SLEN(str) (sizeof(str) - 1)

#define msetstr(dst, src) do {   \
	memcpy(dst, src, SLEN(dst)); \
	dst[SLEN(dst)] = '\0';       \
} while(0)

#define is_start_with(src, str, len) \
	(!strncmp(src, str, len))

#define is_start_with_s(src, str) \
	is_start_with(src, str, SLEN(str))

#define is_end_with(src, len, str, slen) \
	(!strncmp(src + len - slen, str, slen))

#define is_end_with_s(src, len, str) \
	is_end_with(src, len, str, SLEN(str))

#define is_start_with_end_with_s(src, len, prefix, suffix) \
	is_start_with_s(src, prefix) && is_end_with_s(src, len, suffix)

#define is_start_with_end_with(src, len, prefix, prefix_len, suffix, suffix_len) \
	is_start_with(src, prefix, prefix_len) && is_end_with(src, len, suffix, suffix_len)

#ifdef __cplusplus
}
#endif

#endif
