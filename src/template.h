#ifndef __UI_TEMPLATE_H_
#define __UI_TEMPLATE_H_

#include "define.h"
#include "search.h"

#include "core.h"

#include <ctype.h>


#define MAX_HIGH_LIGHT_TAG_LEN WORD_SIZE

#define HIGH_LIGHT_START_TAG "<span class=\"kwcolor\">"
#define HIGH_LIGHT_END_TAG "</span>"


#define		MAX_PAGETEXT_LEN    40960
#define   	MAX_RES_TEMPL_SIZE  4096
#define   	MAX_RES_VARS_NUM    32
#define   	MAX_VARS_NUM	    128

#define 	MAX_VAR_NAME_LEN  	32
#define 	MAX_ACTION_LEN		32
#define 	MAX_TIME_FMT_LEN	32

#define		MAX_AS_ABSTRACT_LEN       150


#define		MAX_QUERY_TERMS_NUM 32
#define		MAX_QUERY_TERM_LEN	64

typedef struct tag_as_query_term_t
{
	//词条
	char term[MAX_QUERY_TERM_LEN];
	//词长度
	int len;
}as_query_term_t;

typedef struct
{
	int num;
	as_query_term_t query_term[MAX_QUERY_TERMS_NUM + 1];
}as_query_t;

#define MAX_HIGH_LIGHT_TERM_NUM 128
typedef struct tag_as_temp_high_light_t
{
	int offset;
	int len;
}as_temp_high_light_t;


//模板变量对应能得到的值的来源
typedef enum
{
	error_field,
	none_exist_field,
	not_need_field,

	result_head_status,
	result_head_total_num,
	result_head_return_num,
	result_head_query_term,

	result_node_key1,
	result_node_key2,
	result_node_attr_int,
	result_node_attr_discrete,
	result_node_short_attr,
	result_node_long_attr,
	result_node_sort_weight,
	result_node_is_del,
	result_node_abstract,

	as_request_query,
	as_request_field_type,
	as_request_attr_int,
	as_request_attr_discrete,
	as_request_return_num,
	as_request_sort_field,
	as_request_sort_type,
	as_request_need_del,
	as_request_must_and,
	as_request_need_synonym,
	as_request_pid_str,

	session_t_server_timeused,

	currnet_timestamp,

	//每页返回的结果数
	as_apache_rn,
	//uiLanguage
	as_apache_language,
	//uiPageNumInt
	as_apache_pn,
	//uiQuestionType
	as_apache_qt,
	//uiSearchName
	as_apache_sn,
	//hight_light
	as_apache_ht,
	//pageCount
	as_page_count,
	//sort
	as_apache_sort,
}var_attr_from_t;

/**
*
*	为AS_SEARCH定义新的变量转换的方法
*	luodonghshan@tianya.cn
*/

typedef enum tag_var_trans_method_t
{
	unknow_trans_method,
	not_trans,
	trans_method_notneed,
	//字符串调用conv方法来转换,uiQueryWord采用此方法
	//as_temp_string_conv_method方法
	str_to_conv,
	str_to_conv_quote_escape,
	str_uri_encode,
	str_uri_encode_utf8,
	str_xml_encode,
	uint_to_uint,
	str_to_str,
	str_full_high_light,
	uint_to_time_used,
	uint_to_language_str,
	//uiTotalNumStr
	uint_to_total_number_str,
	str_title_type_basic,
	str_abstract_type_basic,
	str_abstract_type_wendabasic,
	str_title_type_cut,
	uint_to_time_str,
	uint_to_javascript_time,
	str_to_escape_backslash,
	str_to_pid_name,
	uint_to_pidstr,
	uint_to_flag,
	str_to_json_term,
	str_high_light_basic,

	//问答特别有的变量
	str_to_wenda_topic_id,
	str_to_wenda_lable_ids,
}var_trans_method_t;

typedef enum
{
	field_type_unknow,
	field_type_uint,
	field_type_char,
	field_type_page_num,
	field_type_page_pre,
	field_type_page_next,
	//相关搜索词
	field_type_relation,
	//相关版块
	field_type_relative,
	field_type_relative_json,
	//与关键词名相关的版块
	field_type_relative_pids,
	field_type_relative_pids_json,
	field_type_sort,
	field_type_sort_menu,
	field_type_time_menu,
	field_type_css,
	field_type_results,
	field_type_index_top,
	field_type_err_msg,
	field_type_url,
	field_type_plate,
	field_type_uistatus,
	//result_node_0
	result_node_0,
	field_type_delete_last_space,
	field_type_pidurl,
	field_type_qiye,
	field_type_relative_topics,
}var_attr_from_type_t;



typedef struct tag_Var_attr_t
{
	u_int  		pos;       			 		//变量在该buf中所处的起始位置
	u_int  		len;       			 		//该变量的长度(包括$$在内)
	int			id;         				//变量在系统中的ID
	char 		name[MAX_VAR_NAME_LEN];   	//模板变量名

	char 		fieldname[MAX_VAR_NAME_LEN];//程序里结果集里的字段名
	var_attr_from_t from_id;//此id通过fieldname查找名字对应出来
	var_attr_from_type_t from_field_type;			//为方便写程序添加的,来源字段的类型  0  uint数字型  1 字符串类型 其它等定义
	int			from_id_idx;//如果from_id对应的是一个数据，则由此idx指出数组的下标值
	char		trans_method_name[MAX_VAR_NAME_LEN];//数据转换方法的名称
	var_trans_method_t		trans_method_id;
}Var_attr_t;

typedef struct tag_as_template_table_t
{
	int			id;         				//变量在系统中的ID
	char 		name[MAX_VAR_NAME_LEN];   	//模板变量名
	char 		fieldname[MAX_VAR_NAME_LEN];//程序里结果集里的字段名
	var_attr_from_t from_id;//此id通过fieldname查找名字对应出来
	var_attr_from_type_t from_field_type;			//为方便写程序添加的,来源字段的类型  0  uint数字型  1 字符串类型 其它等定义
	int			from_id_idx;//如果from_id对应的是一个数据，则由此idx指出数组的下标值
	char		trans_method_name[MAX_VAR_NAME_LEN];//数据转换方法的名称
	var_trans_method_t		trans_method_id;
}as_template_table_t;


typedef struct tag_Template_t
{
	int             charset;
	int				enable_digest;
	int				escape_backslash;
	char            action_str[MAX_ACTION_LEN];
	char			strftime_fmt[MAX_TIME_FMT_LEN];
	char            highlight_start_tag[MAX_HIGH_LIGHT_TAG_LEN];
	char            highlight_end_tag[MAX_HIGH_LIGHT_TAG_LEN];

	char            pagebuf[MAX_PAGETEXT_LEN];
	int            	npv;
	Var_attr_t   	page_vars[MAX_VARS_NUM];
	char            resbuf[MAX_RES_TEMPL_SIZE];
	int             nrv;
	Var_attr_t    	res_vars[MAX_RES_VARS_NUM];
	char 			nonebuf[MAX_PAGETEXT_LEN];
	int				nnv;
	Var_attr_t		none_vars[MAX_VARS_NUM];
	char 			errbuf[MAX_PAGETEXT_LEN];
	int				nev;
	Var_attr_t		err_vars[MAX_VARS_NUM];
}Template_t;

typedef struct tag_Template_handle_t
{
	Template_t tpl[MAX_TEMPLATE_NUM];
	char tn[MAX_TEMPLATE_NUM][MAX_TEMPLATE_NAME_LEN];
	int tn_num;
}Template_handle_t;

#define TEMPLATE_VAR_NUM 38
typedef enum
{
	uiResults = 0,
	uiQueryWord,
	uiPid,
	uiRequestNum,
	uiTemplateName,
	uiTotalNum,
	uiPageNum,
	uiPrePage,
	uiNextPage,
	uiTimeUsed,
	uiIfHighLight,

	uiRelativeMenu,
	uiRelativeMenuJSON,
	uiSortMenu,
	uiCtimeMenu,
	uiSort,
	uiField,
	uiLanguageInt,
	uiLanguage,

	uiResNum,
	uiQuestionType,
	uiSearchName,
	uiPageNumInt,
	uiTotalNumStr,
	uiRelativePids,
	uiRelativePidsJSON,
	uiErrMsg,
	uiQueryWordURIEnc,
	uiQueryWordURIEncUTF8,
	uiQueryWordXMLEnc,
	uiIndexTop,
	uiPidstr,
	uiCurTimestamp,
	uiUiStatus,
	uiReres,
	uiPageCount,
	uiBackspace,
	uiResStr
}Common_page_var;

#define RES_VAR_NUM  40
typedef enum
{
	resTitle = 0,
	resUrl,
	resWriterName,
	resWriterNameColor,
	resWriterID,
	resCreatTime,
	resReplyTime,
	resReplyCounter,
	resClickCounter,
	resTopicID,
	resCreatJavascriptTime,
	resFlag,
	resOtherStr,
	resOther2Str,
	resPidStr,
	resStatus,
	resDigest,
	resPidName,
	resPidUrl,
	resPlate,
	resCutTitle,
	resID,
	resCSS,
	resWeight,

        //根据引擎内部字段名来命名
        resKey1,
        resKey2,
        resAttrInt1,
        resAttrInt2,
        resAttrInt3,
        resAttrInt4,
        resAttrInt5,
        resAttrInt6,
        resAttrDiscrete,
        resShortStr1,
        resShortStr2,
        resLongStr1,
        resLongStr2,
        resSortWeight,
        resIsDel,
        resAbstract
}Res_page_var;

#define temp_get_action_str(_ptpl) _ptpl->action_str
#define temp_get_high_light_start_tag(_ptpl) _ptpl->highlight_start_tag
#define temp_get_high_light_end_tag(_ptpl) _ptpl->highlight_end_tag
#define temp_get_strftime_fmt(_ptpl) _ptpl->strftime_fmt
#define temp_get_enable_digest(_ptpl) _ptpl->enable_digest

extern int template_init();

extern int template_load(char *path);

extern int temp_make_page(int index, char *page, int size, conn_ctx_t *ctx, int iserr);

extern int temp_get_template_index(const char *tn);
extern Template_t *temp_get_template_ptr(int index);

#endif
