#include "template.h"
#include "conf.h"

Template_handle_t *g_pTemplate = NULL;
extern Conf_t g_conf;

static const char *uivar[] = {
   "Results", 	  "QueryWord", 	 "Pid", 	"RequestNum", 	"TemplateName",
   "TotalNum",   "PageNum", 	 "PrePage",	"NextPage", 	"TimeUsed",
   "IfHighLight","RelativeMenu","RelativeMenuJSON", "SortMenu","CtimeMenu","Sort",         "Field",
   "LanguageInt","Language",    "ResNum",  "QuestionType", "SearchName",
   "PageNumInt", "TotalNumStr", "RelativePids", "RelativePidsJSON", "ErrMsg",
   "QueryWordURIEnc","QueryWordURIEncUTF8","QueryWordXMLEnc",
   "IndexTop",   "Pidstr",      "CurTimestamp", "UiStatus", "Reres",
   "PageCount", "Backspace"
};

static int uivar_num = sizeof(uivar) / sizeof(uivar[0]);

static const char *resvar[] = {
   "Title",		"Url", 			"WriterName","WriterNameColor", "WriterID", "CreatTime", "ReplyTime",
   "ReplyCounter", "ClickCounter", "TopicID",    "CreatJavascriptTime",
   "Flag",         "OtherStr",   "Other2Str",    "PidStr",   "Status",
   "Digest",       "PidName",    "PidUrl",		  "Plate",    "CutTitle",
   "ID",			"Css",			"Weight",
   "Key1", "Key2", "AttrInt1", "AttrInt2", "AttrInt3", "AttrInt4", "AttrInt5", "AttrInt6",
   "AttrDiscrete", "ShortStr1", "ShortStr2", "LongStr1", "LongStr2", "SortWeight",
   "IsDel", "Abstract"
};
static int resvar_num = sizeof(resvar) / sizeof(resvar[0]);

as_template_table_t template_table[MAX_VARS_NUM + 1];
//加载模板对应到程序变量及数字处理方法的关系表
static int as_load_template_var_table(char *path);
//为模板项里的每一个模板变量设置相应的处理方法
//根据模板变量名从template_table数据里查找出处理方法
//写入到模板Template_handle_t对应的模板变量里
static int as_set_method_for_template(Template_t *ptemplate);
//为vars数据的vnum个Var_attr_t对象设置处理方法
static int as_set_method_for_vars(Var_attr_t *vars, int vnum);
int as_temp_string_conv_method(char *pbuf, int nleft,
		Template_t *ptemplate, char *srcstr, char *conv_tmp, int tempsize);
int as_char_value_to_other_process(char *value, var_trans_method_t trans_method,
		char *res_value, int res_size, Template_t *ptemplate,
		web_request_t *p_web_req, as_query_t *query_t);
static int urlencode(const char *src, char *dest)
{
	return 0;
}
//对应关系表的实际大小
static int template_table_real_vars_num = 0;

static const char *code_name[5] = { "ENG","GB","BIG5","UTF8","JPKR" };
#define get_code_name(codetype) ((codetype<5 && codetype>=0)? code_name[codetype] : "NONE")

/* smart conv */
#define ENABLE_CONV 0

#if ENABLE_CONV
# define sconv(p, ptemplate, s, o, size) do { \
	int len_s = strlen(s); int size_s = sizeof(s); \
	if (ptemplate->charset != CODETYPE_GB && \
			-1 != ty_conv_zh(s, len_s>size_s?size_s:len_s, o, size, CODETYPE_GB, ptemplate->charset, CODE_TSLT_ZH_CN))\
	p = o;\
	else p = s;\
} while(0)
#else
# define sconv(p, ptemplate, s, o, size) do { \
	p = s;\
	o[0] = '\0';\
} while(0)
#endif


int temp_cutdown_with_more(char *s, char *o, size_t size, int len);
int temp_cutdown(char *s, char *o, size_t osize, int len);


int template_init(void)
{
	g_pTemplate = (Template_handle_t *)malloc(sizeof(Template_handle_t));
	if (g_pTemplate == NULL) {
		WARNING_LOG("malloc template error!");
		return -1;
	}
	memset(g_pTemplate, 0, sizeof(Template_handle_t));
	return 0;
}

int temp_search_uivar(char *str)
{
	for(int i = 0; i < uivar_num; i++) {
		if(strcmp(str, uivar[i]) == 0)
			return i;
	}
	return -1;
}

int temp_search_resvar(char *str)
{
	for(int i = 0; i < resvar_num; i++) {
		if(strcmp(str, resvar[i]) == 0)
			return i;
	}
	return -1;
}

int temp_search(char *str, char **ptpl, int len)
{
	for(int i=0;i<len;i++) {
		if(strcmp(str,ptpl[i])==0)
			return i;
	}
	return -1;
}

int temp_load_res_file(char *path, char *pcontent, Var_attr_t *pvars)
{
	if (path == NULL || pcontent == NULL || pvars == NULL) {
		WARNING_LOG("invalid paremeter !");
		return -1;
	}

	char *pstart;
	char *pend;
	char *p = pcontent;
	Var_attr_t *pvtemp;

	char buf[1024];

	int len, i, num;

	FILE *fp;
	if ((fp = fopen(path, "r")) == NULL) {
		WARNING_LOG("open file %s failed!", path);
		return -1;
	}

	p = pcontent;
	len = 0;
	while(fgets(buf,sizeof(buf),fp)!=NULL) {
		//whether out of memory
		i=strlen(buf);
		len += i;
		if(len >= MAX_RES_TEMPL_SIZE) {
			WARNING_LOG("file %s too big!",path);
			fclose(fp);
			return -1;
		}

		snprintf(p,i+1,"%s",buf);
		p += i;
	}
	fclose(fp);

	// parse the content
	pstart = pcontent;
	num = 0;
	while (pstart != NULL) {
		if (num >= MAX_VARS_NUM) {
			WARNING_LOG("too many vars in template, ignored!");
			break;
		}

		// get a var
		if ((pstart = strchr(pstart, '$')) == NULL)
			break;
		if ((pend = strchr(pstart+1, '$')) == NULL)
			break;

		//if valid var
		if (strncmp(pstart,"$res", 4) != 0 || pend - pstart > MAX_VAR_NAME_LEN) {
			DEBUG_LOG("a var invalid !");
		}

		*pend = '\0';
		if ((i = temp_search_resvar(pstart + 4)) < 0) {
			pstart = pend;
			DEBUG_LOG("a var invalid in resvar!");
			*pend = '$';
			continue;
		}
		*pend = '$';

		pvtemp = &(pvars[num]);
		pvtemp->pos = pstart - pcontent;
		pvtemp->len = pend - pstart + 1;
		pvtemp->id = i;
		snprintf(pvtemp->name, sizeof(pvtemp->name), "$res%s$", resvar[i]);
		num++;
		pstart = pend + 1;
	}

	return num;
}

int temp_load_content_file(char *path, char *pcontent, Var_attr_t *pvars)
{
	if (path == NULL || pcontent == NULL || pvars == NULL) {
		WARNING_LOG("invalid parameters ! ");
		return -1;
	}

	char *pstart;
	char *pend;
	char *p = pcontent;
	Var_attr_t *pvtemp;

	char buf[1024];

	int len, i, num;

	FILE *fp;
	if ((fp = fopen(path, "r")) == NULL) {
		WARNING_LOG("open file[%s] failed!", path);
		return -1;
	}

	p = pcontent;
	len = 0;
	while(fgets(buf,sizeof(buf),fp)!=NULL) {
		//whether out of memory
		i=strlen(buf);
		len += i;
		if(len >= MAX_PAGETEXT_LEN) {
			WARNING_LOG("file[%s] is too big!",path);
			fclose(fp);
			return -1;
		}
		//copy to pcontent
		snprintf(p,i+1,"%s",buf);
		//loop next
		p += i;
	}
	fclose(fp);

	// parse the content
	pstart = pcontent;
	num = 0;
	while (pstart != NULL) {
		if (num >= MAX_VARS_NUM) {
			WARNING_LOG("too many vars in template, ignored!");
			break;
		}

		// get a var
		if ((pstart = strchr(pstart, '$')) == NULL)
			break;
		if ((pend = strchr(pstart+1, '$')) == NULL)
			break;

		//if valid var
		if (strncmp(pstart,"$ui", 3) != 0 || pend - pstart > MAX_VAR_NAME_LEN) {
			DEBUG_LOG("a var invalid !");
			pstart++;
			continue;
		}

		*pend = '\0';
		if ((i = temp_search_uivar(pstart + 3)) < 0) {
			pstart = pend;
			WARNING_LOG("a var invalid in uivar!");
			*pend = '$';
			continue;
		}
		*pend = '$';

		pvtemp = &(pvars[num]);
		pvtemp->pos = pstart - pcontent;
		pvtemp->len = pend - pstart + 1;
		pvtemp->id = i;
		snprintf(pvtemp->name, sizeof(pvtemp->name), "$ui%s$", uivar[i]);
		num++;
		pstart = pend + 1;
		DEBUG_LOG("file[%s] name[%s]", path, pvtemp->name);
	}

	return num;
}

static int readlines_2_array(char *full_path, char *array, int max_item_len, int max_item_num)
{
	FILE *fp = fopen(full_path, "r");
	if (fp == NULL) {
		WARNING_LOG("can not open file [%s]", full_path);
		return -1;
	}
	int num = 0;
	max_item_num--;

	while (fgets(array, max_item_len, fp) != NULL) {
		if (array[0] == '#') {
			continue;
		}
		array[max_item_len - 1] = 0;
		trimNR(array);
		array += max_item_len;
		num++;
		if (num > max_item_num) {
			break;
		}
	}
	fclose(fp);
	return num;
}

int template_load_conf(char *full_path)
{
	return readlines_2_array(full_path, g_pTemplate->tn[0], MAX_TEMPLATE_NAME_LEN, MAX_TEMPLATE_NUM);
}

static void temp_conf_init(Template_t *ptpl)
{
	ptpl->action_str[0] = '\0';
	strncpy(ptpl->highlight_start_tag, HIGH_LIGHT_START_TAG, MAX_HIGH_LIGHT_TAG_LEN);
	ptpl->highlight_start_tag[MAX_HIGH_LIGHT_TAG_LEN-1] = 0;
	strncpy(ptpl->highlight_end_tag, HIGH_LIGHT_END_TAG, MAX_HIGH_LIGHT_TAG_LEN);
	ptpl->highlight_end_tag[MAX_HIGH_LIGHT_TAG_LEN-1] = 0;
	ptpl->escape_backslash = 0;
	ptpl->enable_digest = g_conf.enable_digest;
	sprintf(ptpl->strftime_fmt, "%s", "%F %R");
}

static void temp_assign_conf_item(void *data, char *key, int key_len, char *value)
{
	Template_t *ptpl = (Template_t *)data;
#define CONF_INT(name, item)                 \
	if (!strncasecmp(key, name, key_len)) {  \
		item = atoi(value);					 \
		return;                            \
	}

#define CONF_STR(name, item)                                   \
	if (!strncasecmp(key, name, key_len)) {                    \
		memcpy(item, value, sizeof(item) - 1);                 \
		item[sizeof(item) - 1] = '\0';                         \
		return;                                              \
	}

	CONF_STR("action", ptpl->action_str);
	CONF_STR("high_light_start_tag", ptpl->highlight_start_tag);
	CONF_STR("high_light_end_tag", ptpl->highlight_end_tag);
	CONF_INT("escape_backslash", ptpl->escape_backslash);
	if (g_conf.enable_digest) {
		CONF_INT("enable_digest", ptpl->enable_digest);
	}
	CONF_STR("strftime_fmt", ptpl->strftime_fmt);
#undef CONF_INT
#undef CONF_STR
}

int temp_load_config_file(const char *path, const char *filename, Template_t *ptpl)
{
	assert(path != NULL);
	assert(filename != NULL);
	assert(ptpl != NULL);

	if (ptpl == NULL) {
		WARNING_LOG("invalid paremeter Template_t NULL!");
		return -1;
	}

	temp_conf_init(ptpl);

	if (path == NULL || filename == NULL) {
		WARNING_LOG("invalid paremeter path info!");
		return -1;
	}

	int rv;
	char name[1024];

	snprintf(name, sizeof(name)-1, "%s/%s", path, filename);
	rv = async_load_conf(name, ptpl, temp_assign_conf_item);
	if (rv < 0) {
		DEBUG_LOG("read conf [%s/%s] failed!", path, filename);
	}

	DEBUG_LOG("[action=%s]", ptpl->action_str);
	DEBUG_LOG("[highlight_start_tag=%s]", ptpl->highlight_start_tag);
	DEBUG_LOG("[highlight_end_tag=%s]", ptpl->highlight_end_tag);
	DEBUG_LOG("[escape_backslash=%d]", ptpl->escape_backslash);
	DEBUG_LOG("[enable_digest=%d]", ptpl->enable_digest);
	DEBUG_LOG("[strftime_fmt=%s]", ptpl->strftime_fmt);
	return rv;
}

int template_load(char *path)
{
	char full_path[MAX_PATH_LEN];

	if (path == NULL) {
		WARNING_LOG("invalid parameter!");
		return -1;
	}

	if (g_pTemplate == NULL) {
		WARNING_LOG("g_pTemplate is NULL" );
		return -1;
	}

	//加载通用模板对应关系表
	int ret = as_load_template_var_table(path);
	if (ret < 0) {
		WARNING_LOG("as_load_template_var_table load failed!");
		return -1;
	}

	snprintf(full_path, MAX_PATH_LEN, "%s/template.conf", path);
	int tn_num = template_load_conf(full_path);
	if (tn_num < 0) {
		WARNING_LOG("load template.conf failed!");
		return -1;
	}
	int num = 0,
		i = 0,
		j = 0;
	for (i = 0; i < tn_num; i++) {
		num = 0;
		snprintf(full_path, MAX_PATH_LEN,  "%s/%s/page.html", path, g_pTemplate->tn[i]);
		num = temp_load_content_file(full_path, g_pTemplate->tpl[i].pagebuf, g_pTemplate->tpl[i].page_vars);
		if (num < 0) {
			WARNING_LOG("load content file %s failed!", full_path);
			return -1;
		}
		g_pTemplate->tpl[i].npv = num;

		/* add by xudejian for page charset 2010.04.23 begin*/
		g_pTemplate->tpl[i].charset = 0;
		DEBUG_LOG("tn %s, charset:%d", g_pTemplate->tn[i], g_pTemplate->tpl[i].charset);
		/* add by xudejian 2010.04.23 end*/

		snprintf(full_path, MAX_PATH_LEN, "%s/%s/none.html", path, g_pTemplate->tn[i]);
		num = temp_load_content_file(full_path, g_pTemplate->tpl[i].nonebuf, g_pTemplate->tpl[i].none_vars);
		if (num < 0) {
			WARNING_LOG("load content file %s failed!", full_path);
			return -1;
		}
		g_pTemplate->tpl[i].nnv = num;

		snprintf(full_path, MAX_PATH_LEN, "%s/%s/error.html", path, g_pTemplate->tn[i]);
		num = temp_load_content_file(full_path, g_pTemplate->tpl[i].errbuf, g_pTemplate->tpl[i].err_vars);
		if (num < 0) {
			WARNING_LOG("load content file %s failed!", full_path);
			return -1;
		}
		g_pTemplate->tpl[i].nev = num;

		snprintf(full_path, MAX_PATH_LEN, "%s/%s/res.html", path, g_pTemplate->tn[i]);
		num = temp_load_res_file(full_path, g_pTemplate->tpl[i].resbuf, g_pTemplate->tpl[i].res_vars);
		if (num < 0) {
			WARNING_LOG("load res file %s failed!", full_path);
			return -1;
		}
		g_pTemplate->tpl[i].nrv = num;

		snprintf(full_path, MAX_PATH_LEN,  "%s/%s", path, g_pTemplate->tn[i]);
		temp_load_config_file(full_path, "config.conf", &(g_pTemplate->tpl[i]));

		if (g_pTemplate->tpl[i].enable_digest) {
			g_pTemplate->tpl[i].enable_digest = 1;
			num = g_pTemplate->tpl[i].nrv;
			for (j=0; j<num; j++) {
				if (g_pTemplate->tpl[i].res_vars[j].id == resDigest) {
					g_pTemplate->tpl[i].enable_digest = 2;
					break;
				}
			}
			if (g_pTemplate->tpl[i].enable_digest != 2) {
				g_pTemplate->tpl[i].enable_digest = 0;
			}
		}
		int ret = as_set_method_for_template( &(g_pTemplate->tpl[i]) );
		if (ret < 0) {
			WARNING_LOG("as_set_method_for_template[template:%s]!", g_pTemplate->tn[i]);
			return -1;
		}
	}
	g_pTemplate->tn_num = tn_num;
	return 0;
}

Template_t *temp_get_template_ptr(int index)
{
	if (index < 0 || index >= g_pTemplate->tn_num) {
		return NULL;
	}
	return &(g_pTemplate->tpl[index]);
}

int temp_get_template_index(const char *tn)
{
	int i;

	assert(g_pTemplate != NULL);
	assert(tn != NULL);

	if (g_pTemplate == NULL || tn == NULL) {
		return -1;
	}

	for(i = 0; i < g_pTemplate->tn_num; i++) {
		if(strcasecmp(g_pTemplate->tn[i], tn)==0) {
			return i;
		}
	}

	return -1;
}

int temp_sort_high_light(Search_result_t *pres)
{
	ty_title_result_term_t tmp;
	for (u_int i = 0; i < pres->node.hl_num - 1; i++) {
		u_int n = i;
		for (u_int j = i+1; j < pres->node.hl_num; j++) {
			if (pres->node.highlight_term[n].offset >  pres->node.highlight_term[j].offset)
				n = j;
		}

		if (n != i) {
			memcpy(&tmp, &(pres->node.highlight_term[i]), sizeof(ty_title_result_term_t));
			memcpy(&(pres->node.highlight_term[i]), &(pres->node.highlight_term[n]), sizeof(ty_title_result_term_t));
			memcpy(&(pres->node.highlight_term[n]), &tmp, sizeof(ty_title_result_term_t));
		}
	}
	return 0;
}

int temp_make_high_light(char *page, int nleft, Search_result_t *pres,
		const char *hilight_start_tag, const char *hilight_end_tag)
{
	int n1 = 0;
	char *pbuf = page;
	char cutdown_tmp[1024];
	char *p = NULL;
	int plen = 0;

	memset(cutdown_tmp, 0, sizeof(cutdown_tmp));
	plen = temp_cutdown_with_more(pres->info.title, cutdown_tmp, sizeof(cutdown_tmp), g_conf.title_length);
	if (plen == -1) {
		p = pres->info.title;
		plen = strlen(p);
	} else {
		p = cutdown_tmp;
	}

	if (pres->node.hl_num == 0) {
		n1 = snprintf(pbuf, nleft, "%s", p);
		return n1;
	}
	temp_sort_high_light(pres);

	char *psrc = p;
	u_int i = 0;
	u_int n = 0;
	while (*psrc != '\0' && nleft > 0) {
		if (i < pres->node.hl_num && n == pres->node.highlight_term[i].offset) {
			n1 = snprintf(pbuf, nleft, "%s", hilight_start_tag);
			if (n1 < 0 || n1 > nleft)
				return -1;
			pbuf += n1;
			nleft -= n1;
			DEBUG_LOG("title[%s] hl[%u] offset[%u] len[%u]", p, pres->node.hl_num,
					pres->node.highlight_term[i].offset,
					pres->node.highlight_term[i].len);
			for (u_int k = 0; *psrc != '\0' && k < pres->node.highlight_term[i].len && nleft > 0; k++) {
				*pbuf = *psrc;
				pbuf++;
				psrc++;
				nleft--;
				n++;
			}
			n1 = snprintf(pbuf, nleft, "%s", hilight_end_tag);
			if (n1 < 0 || n1 > nleft)
				return -1;
			pbuf += n1;
			nleft -= n1;
			i++;
			while(i < pres->node.hl_num && pres->node.highlight_term[i].offset == pres->node.highlight_term[i-1].offset) {
				i++;
			}
		} else {
			*pbuf = *psrc;
			pbuf++;
			psrc++;
			nleft--;
			n++;
		}
	}
	return (pbuf - page);
}

int temp_cutdown_with_more(char *s, char *o, size_t size, int len)
{
	assert(s != NULL);
	assert(o != NULL);
	assert(size > 0);
	assert(len >= 0);

	if (s==NULL || size < 2 || o==NULL) {
		return -1;
	}
	size--;

	int slen = strlen(s);
	int need_with_more = 0;

	if ((size_t)len > size || len <= 0) {
		len = size;
	}

	if (slen <= len) {
		need_with_more = 0;
	} else {
		need_with_more = 1;
	}

	if (need_with_more && ((size_t)len + 3 > size || len <= 0)) {
		len = size-3;
	}

	int i;
	for (i=0; *s != '\0' && i<len;i++) {
		if (*s < 0) {
			if (*(s+1) != '\0') {
				o[i++] = *s++;
				o[i] = *s++;
			} else {
				break;
			}
		} else {
			o[i] = *s++;
		}
	}

	if (need_with_more == 1) {
		o[i++] = '.';
		o[i++] = '.';
		o[i++] = '.';
	}
	o[i] = '\0';

	return i;
}

int temp_cutdown(char *s, char *o, size_t osize, int len)
{
	assert(s != NULL);
	assert(o != NULL);
	assert(osize > 0);
	assert(len > 0);

	if (s==NULL || osize < 2 || o==NULL) {
		return -1;
	}
	osize--;

	if ((size_t)len > osize || len <= 0) {
		len = osize;
	}

	int i;
	for (i=0; *s != '\0' && i<len; i++) {
		if (*s < 0) {
			if (*(s+1) != '\0') {
				o[i++] = *s++;
				o[i] = *s++;
			} else {
				break;
			}
		} else {
			o[i] = *s++;
		}
	}
	o[i] = '\0';

	return i;
}

int temp_intdate_to_str(long intdate, char *res, int res_size, const char *fmt)
{
	time_t utc_time = (time_t) intdate;
	struct tm local_time;
	localtime_r(&utc_time, &local_time);
	return strftime(res, res_size, fmt, &local_time);
}

#define temp_intdate_to_javascript_str(_intdate, _res, _size) \
	temp_intdate_to_str(_intdate, _res, _size, "%F %T")

int temp_itoa(int n, char *s, size_t size, short k)
{
	assert(s != NULL);
	assert(size > 0);

	u_int i,j;
	int sign;
	char v;

	if (s==NULL || size < 1) {
		return 0;
	}

	if (k<1) {
		return snprintf(s, size, "%d", n);
	}

	sign = n;
	if (sign < 0) {
		n = -n;
	}

	i=0;
	do{
		s[i++] = n%10 + '0';
		n /= 10;
		if ( n>0 && i<size && (i+1)%(k+1) == 0) {
			s[i++] = ',';
		}
	}while( n>0 && i<size );

	if (i<size && sign<0) {
		s[i++] = '-';
	}

	if (i<size) {
		s[i]='\0';
		for (j=0, i--; j<i; j++, i--) {
			v = s[j];
			s[j] = s[i];
			s[i] = v;
		}
		return i+j+1;
	} else {
		return snprintf(s, size, "%d", n);
	}
}

int max_page_no(u_int rn, u_int total_num)
{
	if (rn == 0)
		return 0;
	if (total_num > MAX_SEARCH_RES_NUM)
		total_num = MAX_SEARCH_RES_NUM;
	int num = total_num / rn;
	if (total_num % rn != 0)
		num++;
	return num;
}

/**
 *	分解从引擎返回来的querys
 */
int as_temp_split_query_terms(char *query_terms, as_query_t *query_t)
{
	int len;
	char *p;
	char *pf;
	if(NULL == query_t) return -1;
	query_t->num = 0;
	if(NULL == query_terms) return 0;
	len = strlen(query_terms);
	if(0 == len) return 0;
	p = pf = query_terms;
	//	printf("query_terms=%s\n", query_terms);
	WARNING_LOG("query_terms=%s", query_terms);
	while((*p) != '\0') {
		while((*p) != '\0' && (*p) != ' ')p++;
		if((*p) == '\0') break;
		if( (*p) == ' ' && p > pf) {
			*p = '\0';
			strcpy(query_t->query_term[query_t->num].term, pf);
			query_t->query_term[query_t->num].len = strlen(query_t->query_term[query_t->num].term);
			// WARNING_LOG("%s %dxxxxxxxx", query_t->query_term[query_t->num].term, query_t->query_term[query_t->num].len);
			query_t->num++;
			if(query_t->num == MAX_QUERY_TERMS_NUM) return 0;
			*p = ' ';
			p++;
			if(query_t->query_term[query_t->num-1].len == 0) query_t->num--;
		}
		while((*p) == ' ')p++;
		pf = p;
	}
	if(p > pf) {
		strcpy(query_t->query_term[query_t->num].term, pf);
		//printf("%s\n", query_t->query_term[query_t->num].term);
		query_t->query_term[query_t->num].len = strlen(query_t->query_term[query_t->num].term);
		query_t->num++;
	}
	//printf("terms_num:%d\n", query_t->num);
	return 0;
}

//从as_temp_make_high_light修改而来
/**
 *
 *	int left_len	截取后剩下的最大长度
 *
 */
int as_temp_make_high_light(char *page, int nleft, char *value,
		const char *hilight_start_tag, const char *hilight_end_tag, as_query_t *query_t,
		int left_len)
{
	int n1 = 0;
	char *pbuf = page;
	char cutdown_tmp[1024];
	char *p = NULL;
	char *pf = NULL;
	char *pvalue;
	int plen = 0;
	int i;
	int j;
	int offset;
	int qlen;
	char tmpchar;

	as_temp_high_light_t high_light[MAX_HIGH_LIGHT_TERM_NUM];
	as_temp_high_light_t temp_hl;
	int high_light_num = 0;
	int max_high_txt_len = 1023;
	if(left_len < max_high_txt_len) max_high_txt_len = g_conf.high_light_length;

	memset(cutdown_tmp, 0, sizeof(cutdown_tmp));
	plen = temp_cutdown_with_more(value, cutdown_tmp, sizeof(cutdown_tmp), max_high_txt_len);
	if (plen == -1) {
		pvalue = value;
		plen = strlen(pvalue);
	} else {
		pvalue = cutdown_tmp;
	}

	if(0 == query_t->num) {
		n1 = snprintf(pbuf, nleft, "%s", pvalue);
		pbuf += n1;
		nleft -= n1;
		if (n1 < 0 || n1 > nleft)return -1;
		return (pbuf - page);
	}
	for(i = 0; i < query_t->num; i++) {
		pf = pvalue;
		int term_count = 0;
		while(query_t->query_term[i].len > 0 && (p = strcasestrGBK(pf, query_t->query_term[i].term)  ) != NULL) {
			offset = p - pvalue;
			//printf("offset:%d v:%s\n", offset, p);
			qlen = query_t->query_term[i].len;
			for(j = 0; j < high_light_num; j++) {
				//当前找到的词是已经找出的词的一个字集，可以忽略当前找出的词
				if(offset >= high_light[j].offset  &&
						offset + qlen <= high_light[j].offset + high_light[j].len) {
					break;
				}
			}
			if(j == high_light_num) {
				high_light[j].offset = offset;
				high_light[j].len = qlen;
				high_light_num++;
				while(j > 0) {
					//把高亮字段按词排序
					if(high_light[j].offset < high_light[j-1].offset) {
						temp_hl = high_light[j];
						high_light[j] = high_light[j-1];
						high_light[j-1] = temp_hl;
					}
					j--;
				}
				if(high_light_num == MAX_HIGH_LIGHT_TERM_NUM - 1) break;
			}
			pf = p + qlen;
			term_count++;
			if(term_count == 5) break;
		}
		if(high_light_num == MAX_HIGH_LIGHT_TERM_NUM - 1) break;
	}
	if(high_light_num > 0) {
		for(i = 1, j = 0; i < high_light_num; i++) {
			if(high_light[j].len + high_light[j].offset >= high_light[i].offset) {
				//可以合并高亮显示
				int temp_len = high_light[i].len + high_light[i].offset - high_light[j].offset;
				if(temp_len > high_light[j].len) {
					high_light[j].len = temp_len;
					//high_light[i].len + high_light[i].offset - high_light[j].offset;
				}
				//printf("xxxxxxxxxxxj=%d len=%d\n", j, high_light[j].len);
			} else {
				j++;
				high_light[j].len = high_light[i].len;
				high_light[j].offset = high_light[i].offset;
			}
		}
		high_light_num = j + 1;
	}
	p = pf = pvalue;
	for(i = 0; i < high_light_num; i++) {
		//printf("offset:%d len:%d\n", high_light[i].offset, high_light[i].len);

		//拷贝term前边的部分字符
		p = pvalue + high_light[i].offset;
		tmpchar = *p;
		*p = '\0';
		n1 = snprintf(pbuf, nleft, "%s", pf);
		pbuf += n1;
		nleft -= n1;
		*p = tmpchar;
		pf = p;
		if (n1 < 0 || n1 > nleft)return -1;


		//拷贝高亮显示的起始字符串
		n1 = snprintf(pbuf, nleft, "%s", hilight_start_tag);
		pbuf += n1;
		nleft -= n1;
		if (n1 < 0 || n1 > nleft)return -1;

		//拷贝高亮显示的字符串
		p = pvalue + high_light[i].offset + high_light[i].len;
		tmpchar = *p;
		*p = '\0';
		n1 = snprintf(pbuf, nleft, "%s", pf);
		pbuf += n1;
		nleft -= n1;
		*p = tmpchar;
		pf = p;
		if (n1 < 0 || n1 > nleft)return -1;

		//拷贝高亮结束的字符串
		n1 = snprintf(pbuf, nleft, "%s", hilight_end_tag);
		pbuf += n1;
		nleft -= n1;
		if (n1 < 0 || n1 > nleft) return -1;

	}
	//拷贝最后一个term之后剩余的字符串
	n1 = snprintf(pbuf, nleft, "%s", pf);
	pbuf += n1;
	nleft -= n1;
	*p = tmpchar;
	pf = p + 1;
	if (n1 < 0 || n1 > nleft)return -1;

	return (pbuf - page);
}

/**
 *
 *	把uint型的数据转换成目标数据的处理方法
 *	把value转换的结果转换成目标数据格式写入到res_value中
 *	res_size 是res_value最大可接受的字节数
 *	ptemplate 是模板指针
 */
int as_uint_value_to_other_process(u_int value, var_trans_method_t trans_method,
		char *res_value, int res_size, Template_t *ptemplate)
{
	int num = 0;
	switch(trans_method) {
		case uint_to_uint:
			num = snprintf(res_value, res_size, "%u", value);
			break;
		case uint_to_time_used:
			num = snprintf(res_value, res_size, "%2.3f", (float)value/1000);
			break;
		case uint_to_total_number_str:
			num = temp_itoa(value, res_value, res_size, 3);
			break;
		case uint_to_time_str:
			num = temp_intdate_to_str(value, res_value, res_size, temp_get_strftime_fmt(ptemplate));
			break;
		case uint_to_javascript_time:
			num = temp_intdate_to_javascript_str(value, res_value, res_size);
			break;
		case uint_to_language_str:
			num = snprintf(res_value, res_size, "%s", get_code_name(value));
			break;
		case uint_to_pidstr:
			break;
		case uint_to_flag:
			break;
		default:
			num = -1;
			WARNING_LOG("unknown trans_method:%d", trans_method);
			break;
	}
	if (num  < 0)
	{
		WARNING_LOG("unknown trans_method");
		return -1;
	}
	//	printf("num:%d\n", num);
	return num;
}


/**
 *	char *page		数据写入缓存的起始地址
 *	int	 nleft		数据写入缓存的剩余可写大小
 *	char *resbuf	??
 *	int nrv			??
 *	result_node_t *pres	结果集变量结点，一个文档
 *	u_int high_light	是否需要高亮显示
 *	Template_t *ptemplate	模板对象
 *	upstream_response_head_t *upstream_response	结果集的头节点指针
 *	web_request_t *p_web_req	apache的讲求节点指针
 *	u_int resid		??
 */
int as_temp_make_res(char *page, int nleft, char *resbuf, int nrv, Var_attr_t *res_vars,
		result_node_t *presnode, u_int high_light, Template_t *ptemplate,as_query_t *query_t,
		upstream_response_head_t *preshead, web_request_t *papachereq, u_int resid)
{
	Var_attr_t *pvars = res_vars;

	if (pvars == NULL) {
		WARNING_LOG("Var_attr_t pvars is NULL!");
		return -1;
	}
	int n = 0;
	int n1 = 0;
	char *ptpl = resbuf;
	char *pbuf = page;
	char *ppageptr = ptpl;
	char *p;
	char conv_tmp[1024];

	for (int i = 0; i < nrv; i++) {
		n = ptpl + pvars[i].pos - ppageptr;
		if (n > nleft) {
			WARNING_LOG("template is too long");
			return -1;
		}
		strncpy(pbuf,ppageptr,n);
		pbuf += n;
		ppageptr += n;
		nleft -= n;

		switch (pvars[i].from_field_type) {
			//printf("i=%d vars=%s\n", i, pvars[i].name);
			case field_type_uint:
				{
					//把程序变量里的u_int字段类型转化为模板里的字段
					u_int uint_value;
					int uint_flag = 0;
					switch (pvars[i].from_id) {
						case result_node_key1:
							uint_value = presnode->key1;
							break;
						case result_node_key2:
							uint_value = presnode->key2;
							break;
						case result_node_attr_discrete:
							break;
						case result_node_attr_int:
							break;
						case result_head_total_num:
							uint_value = preshead->total_num;
							break;
						case result_node_is_del:
							uint_value = presnode->is_del;
							break;
						case result_node_sort_weight:
							uint_value = presnode->sort_weight;
							break;
						case result_node_0:
							uint_value = 0;//resStatus?
							break;
						default:
							uint_flag = 1;
							WARNING_LOG("unknown pvars[i].from_id:%d", pvars[i].from_id);
							return -1;
							break;
					}
					if(uint_flag == 0) {
						n1 = as_uint_value_to_other_process(uint_value,pvars[i].trans_method_id,
								pbuf, nleft, ptemplate);
						if (n1 < 0 || n1 > nleft)
							return -1;
						pbuf += n1;
						nleft -= n1;
					} else {
						return -1;
					}
					break;
				}
			case field_type_char:
				{
					char* char_value = NULL;
					int char_flag = 0;
					switch (pvars[i].from_id) {
						case result_node_abstract:
							char_value = presnode->abstract;
							break;
						case result_node_short_attr:
							char_value = presnode->short_str[pvars[i].from_id_idx];
							break;
						case result_node_long_attr:
							char_value = presnode->long_str[pvars[i].from_id_idx];
							break;
						default:
							char_flag = 1;
							WARNING_LOG("unknown pvars[i].from_id:%d", pvars[i].from_id);
							return -1;
							break;
					}
					if(char_flag == 0) {
						n1 = as_char_value_to_other_process(char_value, pvars[i].trans_method_id,
								pbuf, nleft, ptemplate, papachereq, query_t);
						if (n1 < 0 || n1 > nleft) {
							WARNING_LOG("as_char_value_to_other_process failed");
							return -1;
						}
						pbuf += n1;
						nleft -= n1;
					}
					break;
				}
			case field_type_url:
				break;
			case field_type_plate:
				break;
			case field_type_pidurl:
				break;
			default:
				WARNING_LOG("unknown pvars[i].from_field_type:%d", pvars[i].from_field_type);
				return -1;
				break;

		}
		ppageptr += pvars[i].len;
	}
	n = strlen(ppageptr);
	if (n > nleft) {
		WARNING_LOG("result page is too long");
		return -1;
	}
	strcpy(pbuf, ppageptr);
	pbuf += n;
	*pbuf = '\0';
	return (pbuf - page);
}


/**
 *
 *	把uint型的数据转换成目标数据的处理方法
 *	把value转换的结果转换成目标数据格式写入到res_value中
 *	res_size 是res_value最大可接受的字节数
 *	ptemplate 是模板指针
 */
int as_char_value_to_other_process(char *value, var_trans_method_t trans_method,
		char *res_value, int res_size, Template_t *ptemplate,
		web_request_t *p_web_req, as_query_t *query_t)
{
	int num = 0;
	char tmp[256];
	char *ptmp;
	char conv_tmp[1024];
	char cutdown_tmp[1024];
	char encode_query[MAX_QUERY_WORD_LEN * 3];
	char *p;
	int ret;
	const char *philight_start_tag = temp_get_high_light_start_tag(ptemplate);
	const char *philight_end_tag = temp_get_high_light_end_tag(ptemplate);
	size_t esclen = 0, use_len = 0;

	switch(trans_method) {
		case str_to_conv:
			num =as_temp_string_conv_method(res_value,res_size, ptemplate, value, conv_tmp, sizeof(conv_tmp));
			break;
		case str_to_conv_quote_escape:
			sconv(p, ptemplate, value, conv_tmp, sizeof(conv_tmp));
			if (ptemplate->escape_backslash) {
				esclen = 0;
				use_len = 0;
				num = quote_escape(p, strlen(p), &esclen, res_value, res_size, &use_len);
				if (num == 0) {
					num = use_len;
				}
			} else {
				num = snprintf( res_value, res_size, "%s", p);
			}
			break;
		case str_uri_encode_utf8:
		case str_uri_encode:
			memset(encode_query, 0, sizeof(encode_query));
			urlencode(value, encode_query);
			num = snprintf(res_value, res_size, "%s", encode_query);
			break;
		case str_xml_encode:
			if (XMLEncode(value, conv_tmp, sizeof(conv_tmp)) == -1) {
				p = value;
			} else {
				p = conv_tmp;
			}
			num = snprintf(res_value, res_size, "%s", p);
			break;
		case str_to_str:
			num = snprintf(res_value, res_size, "%s", value);
			break;
		case str_full_high_light:
			num = snprintf(res_value, res_size, "%s", value);
			break;
			//对应于resTitle
		case str_title_type_basic:
			num = snprintf(res_value, res_size, "%s", value);
			break;
			//对应于一般字符串的高亮
		case str_high_light_basic:
			num = snprintf(res_value, res_size, "%s", value);
			break;
		case str_abstract_type_basic:
			num = snprintf(res_value, res_size, "%s", value);
			break;
		case str_title_type_cut:
			temp_cutdown(value, conv_tmp, sizeof(conv_tmp), 15);
			num = snprintf(res_value, res_size, "%s", conv_tmp);
			break;
		case str_to_escape_backslash:
			p = value;
			if (ptemplate->escape_backslash) {
				esclen = 0;
				use_len = 0;
				num = quote_escape(p, strlen(p), &esclen, res_value, res_size, &use_len);
				if (num == 0) {
					num = use_len;
				}
			} else {
				num = snprintf(res_value, res_size, "%s", p);
			}
			break;
		case str_to_pid_name:
			break;
		default:
			num = -1;
			WARNING_LOG("unknown trans_method:%d", trans_method);
			break;
	}
	if (num  < 0 || num > res_size)
	{
		WARNING_LOG("result page is too long");
		return -1;
	}
	//printf("charnum:%d value:%s\n", num, value);
	return num;
}


/*
 *	TyRe_response_t 相关搜索结构体
 */
int as_temp_make_page(char * page,int size, upstream_response_head_t *upstream_response,
		web_request_t *p_web_req, upstream_request_t *up_request, result_node_t *pres,
		Session_t *p_ui_info, int iserr)
{
	char *ptpl = NULL;
	char *pbuf = page;
	int nvars = 0;
	Var_attr_t *pvars = NULL;
	Template_t *ptemplate = NULL;
	const char *paction_str = NULL;
	int index = p_ui_info->tn_index;
	as_query_t query_t;
	WARNING_LOG("Query:[%s]", up_request->query);

	if (index < 0 || index >= g_pTemplate->tn_num) {
		WARNING_LOG("can not find tn[%s] ip[%u]", up_request->tn, p_ui_info->ip);
		index = 0;// user the default template
		iserr = 1;
	}

	ptemplate = &(g_pTemplate->tpl[index]);

	if (iserr > 0) {
		// make error page
		ptpl = ptemplate->errbuf;
		nvars = ptemplate->nev;
		pvars = ptemplate->err_vars;
	} else if (upstream_response->return_num == 0) {
		// make none page
		ptpl = ptemplate->nonebuf;
		nvars = ptemplate->nnv;
		pvars = ptemplate->none_vars;
	} else {
		ptpl = ptemplate->pagebuf;
		nvars = ptemplate->npv;
		pvars = ptemplate->page_vars;
	}

	paction_str = temp_get_action_str(ptemplate);

	char *ppageptr = ptpl;

	int n = 0;
	int nleft = size;
	int n1 = 0;
	int j = 0;
	char encode_query[MAX_QUERY_WORD_LEN * 3];
	int page_count;

	memset(encode_query, 0, sizeof(encode_query));
	urlencode(up_request->query, encode_query);
	DEBUG_LOG("Query:[%s]", up_request->query);
	page_count = max_page_no(up_request->rn, upstream_response->total_num);
	if (up_request->page_no > page_count) {
		if (page_count > 1) {
			up_request->page_no = page_count - 1;
		} else {
			up_request->page_no = 0;
		}
	}

	for (int i = 0; i < nvars; i++) {
		//printf("i=%d vars=%s\n", i, pvars[i].name);
		n = ptpl + pvars[i].pos - ppageptr;
		if (n > nleft) {
			WARNING_LOG("template is too long");
			return -1;
		}
		strncpy(pbuf,ppageptr,n);
		pbuf += n;
		ppageptr += n;
		nleft -= n;
		//DEBUG_LOG("get var n = %d, tag : %s", pvars[i].id, pvars[i].name);
		//switch (pvars[i].id) {}
		switch (pvars[i].from_field_type) {//通过字段类型来缩小范围

			case field_type_uint:
				{//把程序变量里的u_int字段类型转化为模板里的字段
					u_int uint_value;
					int uint_flag = 0;
					switch (pvars[i].from_id) {
						case result_head_status:
							uint_value = upstream_response->status;
							break;
						case result_head_return_num:
							uint_value = upstream_response->return_num;
							break;
						case result_head_total_num:
							uint_value = upstream_response->total_num;
							break;
						case session_t_server_timeused:
							uint_value = p_ui_info->server_timeused;
							break;
						case currnet_timestamp:
							uint_value = time(0);
							break;
						case as_apache_sort:
							uint_value = up_request->sort;
							break;
						case as_apache_rn:
							uint_value = up_request->rn;//uiRequestNum
							break;
						case as_apache_pn:
							uint_value = up_request->page_no;//uiPageNumInt
							break;
						case as_page_count:
							uint_value = page_count;
							break;
						default:
							WARNING_LOG("unknow type_uint from_id:[%d] varname:%s",
									pvars[i].from_id, pvars[i].name);
							uint_flag = 1;
							break;
					}
					if(uint_flag == 0) {
						n1 = as_uint_value_to_other_process(uint_value,pvars[i].trans_method_id,
								pbuf,nleft,ptemplate);
						if (n1 < 0 || n1 > nleft) {
							WARNING_LOG("as_uint_value_to_other_process failed");
							return -1;
						}
						pbuf += n1;
						nleft -= n1;
					}
					break;
				}
			case field_type_char:
				{
					char* char_value = NULL;
					int char_flag = 0;
					switch (pvars[i].from_id) {
						case as_request_query:
							char_value = up_request->query;
							break;
						case result_head_query_term:
							break;
						default:
							WARNING_LOG("unknow type_char from_id:[%d]", pvars[i].from_id);
							char_flag = 1;
							break;
					}
					if(char_flag == 0) {
						n1 = as_char_value_to_other_process(char_value, pvars[i].trans_method_id,
								pbuf, nleft, ptemplate, p_web_req, &query_t);
						if (n1 < 0 || n1 > nleft) {
							WARNING_LOG("as_char_value_to_other_process failed");
							return -1;
						}
						pbuf += n1;
						nleft -= n1;
					}
					break;
				}
			case field_type_page_num:
				{
					if (up_request->page_no > 6)
						j = up_request->page_no - 6;
					else
						j = 0;
					for (; j < up_request->page_no + 6 && j < page_count; j++)
					{
						if (j == up_request->page_no)
							n1 = snprintf(pbuf, nleft, "<span class=\"current\">%d</span>", j+1);
						else
							n1 = snprintf(pbuf, nleft, "<a href='#'>%d</a>", j+1);
						if (n1 < 0 || n1 > nleft) {
							WARNING_LOG("field_type_page_num failed");
							return -1;
						}
						pbuf += n1;
						nleft -= n1;
					}

					break;
				}
			case field_type_page_pre:
				if (up_request->page_no > 0)
				{
				}
				break;
			case field_type_page_next:
				if (up_request->page_no < page_count - 1)
				{
				}
				break;
			case field_type_sort:
				break;
			case field_type_qiye:
				break;
			case field_type_relative_pids_json:
				break;
			case field_type_relative_topics:
				break;
			case field_type_relative_pids:
				break;
			case field_type_relative_json:
				break;
			case field_type_relative:
				break;
			case field_type_sort_menu:
				break;
			case field_type_time_menu:
				break;
			case field_type_results://需要循环的结果集
				for (u_int k = 0; k < upstream_response->return_num; k++) {

					n1 = as_temp_make_res(pbuf, nleft, ptemplate->resbuf, ptemplate->nrv, ptemplate->res_vars,
							&pres[k], /*p_web_req->high_light*/0, ptemplate,&query_t,
							upstream_response, p_web_req, k);
					if (n1 < 0 || n1 > nleft) {
						WARNING_LOG("field_type_results failed");
						return -1;
					}
					pbuf += n1;
					nleft -= n1;
				}
				break;
			case field_type_delete_last_space://删除当前字符前前一个字符
				if (nleft < size) {
					pbuf--;
					nleft++;
					*pbuf='\0';
				}
				break;
			case field_type_index_top:
				break;
			case field_type_err_msg:
				{
					if (p_ui_info != NULL) {
						if (p_ui_info->merge_status == DISABLE_WORD_ERROR) {
							n1 = snprintf(pbuf, nleft, "根据相关法律法规和政策，部分搜索结果未予显示。");
							if (n1 < 0 || n1 > nleft) {
								WARNING_LOG("field_type_err_msg failed");
								return -1;
							}
							pbuf += n1;
							nleft -= n1;
						}
					}
					break;
				}
			case field_type_uistatus:
				{
					n1 = snprintf(pbuf, nleft, "From Cache: %s;offset: %d", p_ui_info->merge_status == 201 ? "True" : "False",
							p_ui_info->cache_offset);
					if (n1 < 0 || n1 > nleft)
						return -1;
					pbuf += n1;
					nleft -= n1;
					break;
				}
			default:
				WARNING_LOG("unknown pvars[i].from_field_type:%d fieldname:%s",
						pvars[i].from_field_type, pvars[i].fieldname);
				return -1;
				break;
		}
		ppageptr += pvars[i].len;
	}
	n = strlen(ppageptr);
	if (n > nleft)
	{
		WARNING_LOG("result page is too long");
		return -1;
	}
	strcpy(pbuf, ppageptr);
	pbuf += n;
	*pbuf = '\0';

	return (pbuf - page);
}


static int as_set_method_for_vars(Var_attr_t *vars, int vnum)
{
	int i;
	int j;
	char varname[MAX_VAR_NAME_LEN];
	for(i = 0; i < vnum; i++) {
		if(vars[i].name[0] == '$') {
			strcpy(varname, vars[i].name + 1);
		}else {
			strcpy(varname, vars[i].name);
		}
		if(varname[strlen(varname) - 1] == '$') {
			varname[strlen(varname) - 1] = '\0';
		}
		//WARNING_LOG("set method for var:%s %sfailed %d", varname, vars[i].name, template_table_real_vars_num);
		for(j = 0; j < template_table_real_vars_num; j++) {

			if(strcasecmp(varname, template_table[j].name) == 0) {
				strcpy(vars[i].fieldname, template_table[j].fieldname);
				strcpy(vars[i].trans_method_name, template_table[j].trans_method_name);
				vars[i].id = template_table[j].id;
				vars[i].from_id = template_table[j].from_id;
				vars[i].from_id_idx = template_table[j].from_id_idx;
				vars[i].from_field_type = template_table[j].from_field_type;
				vars[i].trans_method_id = template_table[j].trans_method_id;
				break;
			}
		}
		if(j == template_table_real_vars_num) {
			WARNING_LOG("set method for var:%s failed", varname);
			return -1;
		}
	}
	return 0;
}

static int as_set_method_for_template(Template_t *ptemplate)
{
	int ret;
	ret = as_set_method_for_vars(ptemplate->page_vars, ptemplate->npv);
	if(ret < 0) {
		WARNING_LOG("as_set_method_for_vars for page_vars failed");
		return -1;
	}

	ret = as_set_method_for_vars(ptemplate->res_vars, ptemplate->nrv);
	if(ret < 0) {
		WARNING_LOG("as_set_method_for_vars for res_vars failed");
		return -1;
	}

	ret = as_set_method_for_vars(ptemplate->none_vars, ptemplate->nnv);
	if(ret < 0) {
		WARNING_LOG("as_set_method_for_vars for none_vars failed");
		return -1;
	}

	ret = as_set_method_for_vars(ptemplate->err_vars, ptemplate->nev);
	if(ret < 0) {
		WARNING_LOG("as_set_method_for_vars for err_vars failed");
		return -1;
	}
	return 0;
}

/**
 *	通过模板变量名查找模板值的来源，
 *	from_type返回的值为模板变量来源值的数据类型
 *	当返回值为not_need_field时，说明数据是直接根据from_type来处理的
 */
static var_attr_from_t as_get_attr_from_id_by_field_name(char *name, var_attr_from_type_t *from_type)
{
	if(NULL == name) {
		WARNING_LOG("in get_attr_from_id_by_field_name name cannot be NULL");
		return error_field;
	}


	if(strcasecmp(name, "result_head_status") == 0) {
		*from_type = field_type_uint;
		return result_head_status;
	} else if(strcasecmp(name, "result_head_total_num") == 0) {
		*from_type = field_type_uint;
		return result_head_total_num;
	} else if(strcasecmp(name, "result_head_return_num") == 0) {
		*from_type = field_type_uint;
		return result_head_return_num;
	} else if(strcasecmp(name, "result_head_query_term") == 0) {
		*from_type = field_type_char;
		return result_head_query_term;
	}

	else if(strcasecmp(name, "result_node_key1") == 0) {
		*from_type = field_type_uint;
		return result_node_key1;
	} else if(strcasecmp(name, "result_node_key2") == 0) {
		*from_type = field_type_uint;
		return result_node_key2;
	} else if(strcasecmp(name, "result_node_attr_int") == 0) {
		*from_type = field_type_uint;
		return result_node_attr_int;
	} else if(strcasecmp(name, "result_node_attr_discrete") == 0) {
		return result_node_attr_discrete;
	} else if(strcasecmp(name, "result_node_short_attr") == 0) {
		*from_type = field_type_char;
		return result_node_short_attr;
	} else if(strcasecmp(name, "result_node_long_attr") == 0) {
		*from_type = field_type_char;
		return result_node_long_attr;
	} else if(strcasecmp(name, "result_node_sort_weight") == 0) {
		*from_type = field_type_uint;
		return result_node_sort_weight;
	} else if(strcasecmp(name, "result_node_is_del") == 0) {
		*from_type = field_type_uint;
		return result_node_is_del;
	} else if(strcasecmp(name, "result_node_abstract") == 0) {
		*from_type = field_type_char;
		return result_node_abstract;
	}
	else if(strcasecmp(name, "as_request_query") == 0) {
		*from_type = field_type_char;
		return as_request_query;
	} else if(strcasecmp(name, "as_request_field_type") == 0) {
		*from_type = field_type_uint;
		return as_request_field_type;
	} else if(strcasecmp(name, "as_request_attr_int") == 0) {
		*from_type = field_type_uint;
		return as_request_attr_int;
	} else if(strcasecmp(name, "as_request_attr_discrete") == 0) {
		return as_request_attr_discrete;
	} else if(strcasecmp(name, "as_request_return_num") == 0) {
		*from_type = field_type_uint;
		return as_request_return_num;
	} else if(strcasecmp(name, "as_request_sort_field") == 0) {
		*from_type = field_type_uint;
		return as_request_sort_field;
	} else if(strcasecmp(name, "as_request_sort_type") == 0) {
		*from_type = field_type_uint;
		return as_request_sort_type;
	} else if(strcasecmp(name, "as_request_need_del") == 0) {
		*from_type = field_type_uint;
		return as_request_need_del;
	} else if(strcasecmp(name, "as_request_must_and") == 0) {
		*from_type = field_type_uint;
		return as_request_must_and;
	} else if(strcasecmp(name, "as_request_need_synonym") == 0) {
		*from_type = field_type_uint;
		return as_request_need_synonym;
	} else if(strcasecmp(name, "as_request_pid_str") == 0) {
		*from_type = field_type_char;
		return as_request_pid_str;
	}

	else if(strcasecmp(name, "session_t_server_timeused") == 0) {
		*from_type = field_type_uint;
		return session_t_server_timeused;
	}

	else if(strcasecmp(name, "currnet_timestamp") == 0) {
		*from_type = field_type_uint;
		return currnet_timestamp;
	}

	else if(strcasecmp(name, "field_type_relative") == 0) {
		*from_type = field_type_relative;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_relative_json") == 0) {
		*from_type = field_type_relative_json;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_relative_pids") == 0) {
		*from_type = field_type_relative_pids;
		return not_need_field;
	}  else if(strcasecmp(name, "field_type_relative_pids_json") == 0) {
		*from_type = field_type_relative_pids_json;
		return not_need_field;
	}  else if(strcasecmp(name, "field_type_relative_topics") == 0) {
		*from_type = field_type_relative_topics;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_time_menu") == 0) {
		*from_type = field_type_time_menu;
		return not_need_field;
	}  else if(strcasecmp(name, "field_type_sort") == 0) {
		*from_type = field_type_sort;
		return not_need_field;
	}  else if(strcasecmp(name, "field_type_qiye") == 0) {
		*from_type = field_type_qiye;
		return not_need_field;
	}  else if(strcasecmp(name, "field_type_sort_menu") == 0) {
		*from_type = field_type_sort_menu;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_results") == 0) {
		*from_type = field_type_results;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_page_pre") == 0) {
		*from_type = field_type_page_pre;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_page_next") == 0) {
		*from_type = field_type_page_next;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_page_num") == 0) {
		*from_type = field_type_page_num;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_index_top") == 0) {
		*from_type = field_type_index_top;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_relation") == 0) {
		*from_type = field_type_relation;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_err_msg") == 0) {
		*from_type = field_type_err_msg;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_url") == 0) {
		*from_type = field_type_url;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_plate") == 0) {
		*from_type = field_type_plate;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_pidurl") == 0) {
		*from_type = field_type_pidurl;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_uistatus") == 0) {
		*from_type = field_type_uistatus;
		return not_need_field;
	} else if(strcasecmp(name, "as_apache_qt") == 0) {
		*from_type = field_type_uint;
		return as_apache_qt;
	} else if(strcasecmp(name, "as_apache_sn") == 0) {
		*from_type = field_type_uint;
		return as_apache_sn;
	} else if(strcasecmp(name, "as_apache_sort") == 0) {
		*from_type = field_type_uint;
		return as_apache_sort;
	} else if(strcasecmp(name, "as_apache_rn") == 0) {//uiRequestNum
		*from_type = field_type_uint;
		return as_apache_rn;
	} else if(strcasecmp(name, "as_apache_pn") == 0) {//uiPageNumInt
		*from_type = field_type_uint;
		return as_apache_pn;
	} else if(strcasecmp(name, "as_apache_ht") == 0) {//uiIfHighLight
		*from_type = field_type_uint;
		return as_apache_ht;
	} else if(strcasecmp(name, "as_apache_language") == 0) {//uiLanguage
		*from_type = field_type_uint;
		return as_apache_language;
	} else if(strcasecmp(name, "as_page_count") == 0) {//uiPageCount
		*from_type = field_type_uint;
		return as_page_count;
	} else if(strcasecmp(name, "result_node_0") == 0) {//0
		*from_type = result_node_0;
		return not_need_field;
	} else if(strcasecmp(name, "field_type_delete_last_space") == 0) {//delete one char
		*from_type = field_type_delete_last_space;
		return not_need_field;
	}
	return none_exist_field;
}

#define STR(name) #name
#define TM(method) { method, STR(method) }

struct trans_methods_t {
	var_trans_method_t method;
	const char *name;
} trans_methods[] = {
	TM(uint_to_uint),
	TM(trans_method_notneed),
	TM(str_to_conv),
	TM(str_to_conv_quote_escape),
	TM(str_uri_encode),
	TM(str_uri_encode_utf8),
	TM(str_xml_encode),
	TM(str_full_high_light),
	TM(str_to_str),
	TM(uint_to_time_used),
	TM(uint_to_language_str),
	TM(uint_to_total_number_str),
	TM(str_title_type_basic),
	TM(str_high_light_basic),
	TM(str_abstract_type_basic),
	TM(uint_to_time_str),
	TM(uint_to_javascript_time),
	TM(str_to_pid_name),
	TM(str_to_escape_backslash),
	TM(uint_to_pidstr),
	TM(uint_to_language_str),
	TM(uint_to_flag),
	TM(str_to_json_term),
	TM(str_to_wenda_topic_id),
	TM(str_to_wenda_lable_ids),
	{ not_trans, "not_need_field" },
	{ unknow_trans_method, NULL }
};

/**
 *	通过模板变量名对应的转换方法名找到转换方法
 */
static var_trans_method_t as_get_trans_method_by_trans_method_name(char *trans_name)
{
	if(NULL == trans_name) {
		WARNING_LOG("in as_get_trans_method_by_trans_method_name trans_name cannot be NULL");
		return unknow_trans_method;
	}

	int num = sizeof(trans_methods) / sizeof(trans_methods[0]);
	for (int i=0; i< num; i++) {
		if (trans_methods[i].name) {
			if (!strcasecmp(trans_name, trans_methods[i].name)) {
				return trans_methods[i].method;
			}
		}
	}

	return unknow_trans_method;
}




/**
 *
 *	校验各个模板项的合法性
 *	并根据方法的名称为模板设置相应的处理方法
 *
 */
static int as_template_var_valid(as_template_table_t *ptemplate_item)
{
	var_attr_from_type_t field_type = field_type_unknow;
	if(NULL == ptemplate_item) {
		WARNING_LOG("ptemplate_item cannot be NULL");
		return -1;
	}

	//根据fieldname找到对应的字段名
	ptemplate_item->from_id = as_get_attr_from_id_by_field_name(ptemplate_item->fieldname, &field_type);
	ptemplate_item->from_field_type = field_type;
	if(error_field == ptemplate_item->from_id ||
			none_exist_field == ptemplate_item->from_id) {
		WARNING_LOG("as_template_var_valid failed fieldname:%s", ptemplate_item->fieldname);
		return -1;
	}

	//根据trans_method_name名称找到对应的数据转换方法
	ptemplate_item->trans_method_id = as_get_trans_method_by_trans_method_name(ptemplate_item->trans_method_name);
	if(unknow_trans_method == ptemplate_item->trans_method_id) {
		WARNING_LOG("ptemplate_item->trans_method_name[%s]not exist", ptemplate_item->trans_method_name);
		return -1;
	}

	return 0;
}

static int as_load_template_var_table(char *path)
{
	char full_path[MAX_PATH_LEN];
	as_template_table_t *ptemplate_table = template_table;
	int num = 0;
	char *p;
	char *pf;
	char temp_str[1024];
	snprintf(full_path, MAX_PATH_LEN, "%s/template_table.conf", path);
	FILE *fp = fopen(full_path, "r");
	if (fp == NULL) {
		WARNING_LOG("can not open file [%s]", full_path);
		return -1;
	}
	char line[1024];
	while (fgets(line, sizeof(line), fp) != NULL) {
		if (line[0] == '#')
			continue;
		trimNR(line);
		//第一项,模板变量名，配置文件决定
		p = pf = line;
		if ((p = strchr(p, '=')) == NULL) {
			WARNING_LOG("line[%s] one failed", line);
			return -1;
		}
		*p = '\0';
		strcpy(temp_str, pf);
		trimNR(temp_str);
		snprintf(ptemplate_table[num].name, MAX_VAR_NAME_LEN, "%s", temp_str);

		//第二项，程序字段名，程序内部定义
		p++;
		pf = p;
		if ((p = strchr(p, '=')) == NULL) {
			WARNING_LOG("line[%s] two failed", line);
			return -1;
		}
		*p = '\0';
		strcpy(temp_str, pf);
		trimNR(temp_str);
		snprintf(ptemplate_table[num].fieldname, MAX_VAR_NAME_LEN, "%s", temp_str);

		//第三项，程序内部字段下标，配置文件决定
		p++;
		pf = p;
		if ((p = strchr(p, '=')) == NULL){
			WARNING_LOG("line[%s] three failed", line);
			return -1;
		}
		*p = '\0';
		strcpy(temp_str, pf);
		trimNR(temp_str);
		if(!isdigit(temp_str[0]) )continue;
		ptemplate_table[num].from_id_idx = atoi(temp_str);

		//第四项，字段转换关系，程序内部定义
		p++;
		pf = p;
		strcpy(temp_str, pf);
		trimNR(temp_str);
		snprintf(ptemplate_table[num].trans_method_name, MAX_VAR_NAME_LEN, "%s", temp_str);
		if( as_template_var_valid(&ptemplate_table[num]) < 0){
			WARNING_LOG("line[%s] as_template_var_valid failed", line);
			return -1;
		}
		num++;
		if(num == MAX_VARS_NUM) {
			WARNING_LOG("too many vars [%s]", full_path);
			return -1;//too many vars
		}
	}
	fclose(fp);
	template_table_real_vars_num = num;
	if(template_table_real_vars_num == 0) {
		WARNING_LOG("template_table_real_vars_num == 0");
	}
	return 0;
}

/**
 *
 *	把srcstr转换库ptemplate需要的字符集类型，写入到pbuf里，返回写入到pbuf里的字符数量
 *
 ***/
int as_temp_string_conv_method(char *pbuf, int nleft, Template_t *ptemplate, char *srcstr, char *conv_tmp, int tempsize)
{
	int size;
	char *p;
	if(NULL == srcstr) return 0;
	if(NULL == pbuf || NULL == ptemplate || NULL == conv_tmp) return -1;
	sconv(p, ptemplate, srcstr, conv_tmp, tempsize);
	size = snprintf(pbuf, nleft, "%s", p);
	return size;
}
