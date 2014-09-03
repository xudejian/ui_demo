#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "util.h"
#include <iconv.h>
#include <locale.h>
#include <wchar.h>
#include <poll.h>
#include <errno.h>

void trimNR(char *src)
{
	char *p = src;
	if (p == NULL) {
		return;
	}
	while(*p != '\0') {
		p++;
	}
	while(p >= src && (*p == '\n' || *p == '\r' || *p == '\0')) {
		*p = '\0';
		p--;
	}
}

int cstr_split(char *src, int slen, char sep, char **obj, int size)
{
	int num = 0;
	int i = 0;
	if (size < 1) {
		goto SPLIT_FAIL;
	}

	i = 0;
	while(i<slen && src[i] && src[i] == sep) {
		i++;
	}

	if (i<slen && src[i] /*&& src[i] != sep*/) {
		obj[num++] = &(src[i]);
	}

	for (; i<slen && src[i] && num < size; i++) {
		if (src[i] != sep) {
			continue;
		} else {
			src[i] = '\0';
			i++;
			while(i<slen && src[i] && src[i] == sep) {
				i++;
			}

			if (i<slen && src[i] /*&& src[i] != sep*/) {
				obj[num++] = &(src[i]);
			}
		}
	}

	assert(num <= size);
	return num;

SPLIT_FAIL:
	return -1;
}

void str2lower(char *s)
{
	if (s == NULL)
		return;
	char *p = s;
	while(*p) {
		if(*p < 0) {
			p++;
			if(*p == '\0') break;
			p++;
			continue;
		}
		*p = tolower(*p);
		p++;
	}
}

char * strstrGBK(char *src,const char *dest)
{
	char *ptemp = src;
	if(NULL == dest) {
		printf("dest is null\n");return NULL;
	}
	int len = strlen(dest);
	for(; *ptemp; ptemp++) {
		if(*ptemp == *dest && strncmp(ptemp, dest, len)==0) {
			return ptemp;
		}

		if(*ptemp&0x80) {
			ptemp++;
			if(*ptemp=='\0') return NULL;
		}
	}
	return NULL;
}

void gbk_blank_to_en(char *s)
{
	if(NULL == s || s[0] == '\0') return;
	trimNR(s);
	char *p = s;
	char *np = s;
	char *str="　";//中文空格
	while((*p) == ' '){
		p++;
	}
	while((*p) != '\0') {
		if((*p) == str[0] && (*(p+1)) == str[1] ) {
			*np = ' ';
			p++;
		} else if( (*p)&0x80 ) {
			*np = *p;
			p++;
			np++;
			if( (*p) == '\0') {
				break;
			}
			*np = *p;
		}else {
			*np = *p;
		}
		np++;
		p++;
	}
	*np = '\0';
}

char * strcasestrGBK(char *src,const char *dest)
{
	char *ptemp = src;
	int len = strlen(dest);
	for(; *ptemp; ptemp++) {
    if(*ptemp == *dest && strncasecmp(ptemp, dest, len)==0) {
			return ptemp;
		}

		if(*ptemp&0x80) {
			ptemp++;
			if(*ptemp=='\0') return NULL;
		}
	}
	return NULL;
}

int fixstring(char *s, size_t size, int word_len)
{
	assert(s != NULL);
	assert(size > 0);
	assert(word_len > 0);

	if (s==NULL || size < 2) {
		return -1;
	}
	size--;

	if (word_len < 2) {
		word_len = 2;
	}

	int j;
	size_t i = 0;
	while (s[i] != '\0' && i<size) {
		if (s[i] < 0) {
			for (j=1; j<word_len; j++) {
				if (s[i+j] == '\0') {
					s[i] = '\0';
					return i;
				}
			}
			i += word_len;
		} else {
			i++;
		}
	}

	if (i == size) {
		s[i] = '\0';
	}

	return i;
}

#define XML_CHAR_LT '<'
#define XML_CHAR_GT '>'
#define XML_CHAR_QUOT '"'
#define XML_CHAR_DYH '\''
#define XML_CHAR_AND '&'

#define XML_XML_LT "&lt;"
#define XML_XML_GT "&gt;"
#define XML_XML_QUOT "&quot;"
#define XML_XML_DYH "&#39;"
#define XML_XML_AND "&amp;"

#define XML_LEN_LT 4
#define XML_LEN_GT 4
#define XML_LEN_QUOT 6
#define XML_LEN_DYH 5
#define XML_LEN_AND 5

static int XML_Encode_need_size(const char *src)
{
	int size = 0;

	if (src == NULL) {
		return -1;
	}

	while(*src) {
		switch(*src) {
			case XML_CHAR_LT:
				size += XML_LEN_LT;
				break;
			case XML_CHAR_GT:
				size += XML_LEN_GT;
				break;
			case XML_CHAR_QUOT:
				size += XML_LEN_QUOT;
				break;
			case XML_CHAR_DYH:
				size += XML_LEN_DYH;
				break;
			case XML_CHAR_AND:
				size += XML_LEN_AND;
				break;
			default:
				size++;
				break;
		}
		src++;
	}
	return size;
}

int XMLEncode(const char *src, char *obj, int size)
{
	int need_size = 0;
	if (src == NULL || obj == NULL || size < 2) {
		return -1;
	}
	size--;

	need_size = XML_Encode_need_size(src);
	if (size < need_size) {
		return -1;
	}

#define XML_ENC_XML_STR(xmlstr, xmllen) \
	do { \
		memcpy(obj, xmlstr, xmllen);\
		size -= xmllen;\
		obj += xmllen;\
		src++;\
	}while(0);

	while(*src && size) {
		switch(*src) {
			case XML_CHAR_LT:
				XML_ENC_XML_STR(XML_XML_LT, XML_LEN_LT);
				break;
			case XML_CHAR_GT:
				XML_ENC_XML_STR(XML_XML_GT, XML_LEN_GT);
				break;
			case XML_CHAR_QUOT:
				XML_ENC_XML_STR(XML_XML_QUOT, XML_LEN_QUOT);
				break;
			case XML_CHAR_DYH:
				XML_ENC_XML_STR(XML_XML_DYH, XML_LEN_DYH);
				break;
			case XML_CHAR_AND:
				XML_ENC_XML_STR(XML_XML_AND, XML_LEN_AND);
				break;
			default:
				*obj = *src;
				obj++;
				src++;
				size--;
				break;
		}
	}

#undef XML_ENC_XML_STR

	*obj = 0;
	return need_size;
}

static int XML_Decode_need_size(const char *src)
{
	int size = 0;

	if (src == NULL) {
		return -1;
	}

	while(*src) {
		if (*src == '&') {
			if (!strncmp(src, XML_XML_LT, XML_LEN_LT)) {
				src += XML_LEN_LT;
			} else if (!strncmp(src, XML_XML_GT, XML_LEN_GT)) {
				src += XML_LEN_GT;
			} else if (!strncmp(src, XML_XML_QUOT, XML_LEN_QUOT)) {
				src += XML_LEN_QUOT;
			} else if (!strncmp(src, XML_XML_DYH, XML_LEN_DYH)) {
				src += XML_LEN_DYH;
			} else if (!strncmp(src, XML_XML_AND, XML_LEN_AND)) {
				src += XML_LEN_AND;
			} else {
				src++;
			}
		} else {
			src++;
		}
		size++;
	}
	return size;
}

int XMLDecode(const char *src, char *obj, int size)
{
	int need_size = 0;
	if (src == NULL || obj == NULL || size < 2) {
		return -1;
	}
	size--;

	need_size = XML_Decode_need_size(src);
	if (size < need_size) {
		return -1;
	}

#define XML_DEC_XML_STR(xmlstr, xmllen, xmlchar) \
	do { \
		*obj = xmlchar;\
		src += xmllen;\
		obj++;\
		size--;\
	}while(0);

	while(*src && size) {
		if (*src == '&') {
			if (!strncmp(src, XML_XML_LT, XML_LEN_LT)) {
				XML_DEC_XML_STR(XML_XML_LT, XML_LEN_LT, XML_CHAR_LT);
			} else if (!strncmp(src, XML_XML_GT, XML_LEN_GT)) {
				XML_DEC_XML_STR(XML_XML_GT, XML_LEN_GT, XML_CHAR_GT);
			} else if (!strncmp(src, XML_XML_QUOT, XML_LEN_QUOT)) {
				XML_DEC_XML_STR(XML_XML_QUOT, XML_LEN_QUOT, XML_CHAR_QUOT);
			} else if (!strncmp(src, XML_XML_DYH, XML_LEN_DYH)) {
				XML_DEC_XML_STR(XML_XML_DYH, XML_LEN_DYH, XML_CHAR_DYH);
			} else if (!strncmp(src, XML_XML_AND, XML_LEN_AND)) {
				XML_DEC_XML_STR(XML_XML_AND, XML_LEN_AND, XML_CHAR_AND);
			} else {
				*obj = *src;
				src++;
				obj++;
				size--;
			}
		} else {
			*obj = *src;
			src++;
			obj++;
			size--;
		}

	}

#undef XML_DEC_XML_STR

	*obj = 0;
	return need_size;
}

int
no_escape(const char *src, const size_t len, size_t *esclen,
	char *obj, const size_t size, size_t *use_len)
{
	if (size >= len) {
		memcpy(obj, src, len);
		*esclen = len;
		*use_len = len;
	} else {
		memcpy(obj, src, size);
		*esclen = size;
		*use_len = size;
	}
	return 0;
}

int
quote_escape(const char *src, const size_t len, size_t *esclen,
	char *obj, const size_t size, size_t *use_len)
{
	if (src==NULL) return -1;
	if (esclen==NULL) return -1;
	if (obj==NULL) return -1;
	if (use_len==NULL) return -1;

	size_t i = 0,
		j = 0,
		can_push = size - 1;
	unsigned int c = 0;
	char hex_chars[] = "0123456789abcdef";
	if (can_push < 1) return -1;

	*esclen = 0;
	*use_len = 0;

#define QUOTE_ESCAPE_PUSH(_c)  obj[j++] = _c
#define SIZE_CHECK(_push_num)           \
	do {								\
		if (j + _push_num >= can_push) {\
			*esclen = i;                \
			*use_len = j;               \
			QUOTE_ESCAPE_PUSH('\0');    \
			return 0;					\
		}								\
	}while(0)


	for (i=0; i<len; i++) {
		c = (unsigned int)src[i];
		switch (c) {
		case '\\':
		case '"':
			SIZE_CHECK(2);
			QUOTE_ESCAPE_PUSH('\\');
			QUOTE_ESCAPE_PUSH(c);
			break;
		case '/':
			SIZE_CHECK(1);
			QUOTE_ESCAPE_PUSH(c);
			break;
		case '\b':
			SIZE_CHECK(2);
			QUOTE_ESCAPE_PUSH('\\');
			QUOTE_ESCAPE_PUSH('b');
			break;
		case '\t':
			SIZE_CHECK(2);
			QUOTE_ESCAPE_PUSH('\\');
			QUOTE_ESCAPE_PUSH('t');
			break;
		case '\n':
			SIZE_CHECK(2);
			QUOTE_ESCAPE_PUSH('\\');
			QUOTE_ESCAPE_PUSH('n');
			break;
		case '\f':
			SIZE_CHECK(2);
			QUOTE_ESCAPE_PUSH('\\');
			QUOTE_ESCAPE_PUSH('f');
			break;
		case '\r':
			SIZE_CHECK(2);
			QUOTE_ESCAPE_PUSH('\\');
			QUOTE_ESCAPE_PUSH('r');
			break;
		default:
			if (c < ' ') {
				SIZE_CHECK(6);
				QUOTE_ESCAPE_PUSH('\\');
				QUOTE_ESCAPE_PUSH('u');
				QUOTE_ESCAPE_PUSH('0');
				QUOTE_ESCAPE_PUSH('0');
				QUOTE_ESCAPE_PUSH(hex_chars[c >> 4]);
				QUOTE_ESCAPE_PUSH(hex_chars[c & 0xf]);
			} else {
				SIZE_CHECK(1);
				QUOTE_ESCAPE_PUSH(c);
			}
		}
	}

	*esclen = i;
	*use_len = j;
	QUOTE_ESCAPE_PUSH('\0');
#undef QUOTE_ESCAPE_PUSH
#undef SIZE_CHECK
	return 0;
}

int is_socket_epipe(int socket)
{
	struct pollfd pfd;

	pfd.fd     = socket;
	pfd.events = POLLRDHUP | POLLHUP;
	pfd.revents = 0;

	int rv;
	do {
		rv = poll(&pfd, 1, 0);
	} while (rv == -1 && errno == EINTR);

	if (rv == 1) {
		if ((pfd.revents & POLLRDHUP) || (pfd.revents & POLLHUP) || (pfd.revents & POLLERR)) {
			return 1;
		}
		return 0;
	}
	return rv;
}

int is_socket_alive(int socket, int timeout)
{
	fd_set fds;
	struct timeval tv;
	tv.tv_sec = timeout / 1000000;
	tv.tv_usec = timeout % 1000000;

	FD_ZERO(&fds);
	FD_SET(socket, &fds);

	int status = select(socket+1, NULL, &fds, NULL, &tv);
	if(status > 0 && FD_ISSET(socket, &fds)) {
		return 0;
	}
	return -1;
}

#define TEXT_MAX_LEN 1024
int get_gbk_from_utf8(char *dest, char* src)
{
	wchar_t wcharbuf[TEXT_MAX_LEN];
	char temp[TEXT_MAX_LEN + 1];
	int loopi, loopj;
	int src_len;
	int wc_len;
	int status;
	wchar_t wc;
	int count;
	if(NULL == dest || NULL == src) {
		return -1;
	}
	memset(dest, 0, sizeof(dest));
	memset(wcharbuf,0, sizeof(wcharbuf));
	setlocale(LC_ALL, "zh_CN.UTF-8");
	status =  mbstowcs(wcharbuf,src,TEXT_MAX_LEN);
	if(-1== status) {
		src_len = strlen(src);
		for(loopi =0,loopj=0; loopi < src_len;) {
			count = mbtowc(&wc,src+loopi,MB_CUR_MAX);
			if(-1== count) {
				loopi++;
				continue;
			}
			loopi += count;
			wcharbuf[loopj] = wc;
			loopj++;
		}
		if(0==loopj) {
			return -1;
		}
	}
	setlocale(LC_ALL, "zh_CN.GBK");
	status = wcstombs(dest,wcharbuf,TEXT_MAX_LEN);
	if(-1 == status) {
		memset(dest,0,sizeof(dest));
		wc_len = wcslen(wcharbuf);
		for(loopi = 0, loopj = 0;loopi < wc_len;loopi++) {
			memset(temp, 0, sizeof(temp));
			if(-1 == wctomb(temp,wcharbuf[loopi])){
				continue;
			}
			strcat(dest,temp);
		}
	}
	return 0;
}

#ifdef UTIL_DEBUG
#ifdef NDEBUG
#undef NDEBUG
#include <assert.h>
#endif
int main(int argc, char **argv)
{
	unsigned char temp[12];

	/**/
	char *p = argv[1];
	int size = strlen(p);
	size_t esclen = 0;
	size_t use_len = 0;
	int rv = 0;
	int len = 0;
	printf("quote:%s\n", p);
	while(1) {
		rv = quote_escape((const unsigned char *)(p+len), size - len, &esclen, temp, sizeof(temp), &use_len);
		if (use_len == 0) {
			break;
		}
		len += esclen;
		printf("escape:%s\n", temp);
	}

	return 0;
}
#endif
