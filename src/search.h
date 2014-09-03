#ifndef __UI_SEARCH_H_
#define __UI_SEARCH_H_

#define MAX_URL_LEN 256
#define MAX_ABSTRACT_LEN 512
#define MAX_SORT_STR_LEN 32
#define MAX_QUERY_WORD_LEN 12
#define MAX_SEARCH_RES_NUM 750
#define MAX_UI_RETURN_NUM 50


#define MAX_TITLE_LEN 128
#define MAX_WRITER_STR_LEN 64
#define MAX_HIGHTLIGHT_NUM 16
#define MAX_PID_STR_LEN    48
#define MAX_OTHER_STR_LEN 128
#define MAX_TIM_STR_LEN	32


#ifndef u_long
#define u_long unsigned long
#endif

typedef struct _ty_title_info_t
{
	u_int	tid;
	u_int	pid;
	char 	title[MAX_TITLE_LEN];
	char	writer_str[MAX_WRITER_STR_LEN];
	u_int	writerid;
	u_long	create_time;
	u_long	reply_time;
	u_int	click_counter;
	u_int	reply_counter;
	u_int	flag;
	char 	pid_str[MAX_PID_STR_LEN];
	char	other_str[MAX_OTHER_STR_LEN];  //预留
	char  	other2_str[MAX_OTHER_STR_LEN]; //预留
}ty_title_info_t;


typedef struct _ty_title_result_term_t
{
        u_int		sign1;
	    u_int		sign2;
        u_int		offset:16;
        u_int		len:16;
        u_int		t_weight:16;
        u_int		other:16;            //占位，备用
}ty_title_result_term_t;


typedef struct _ty_title_result_node_t
{
	u_int	tid;
	u_int	pid;
	u_int	total_weight;
    u_int   uniq_term_num;
    ty_title_result_term_t highlight_term[MAX_HIGHTLIGHT_NUM];
	u_int	hl_num;
}ty_title_result_node_t;

/*
cgi传给UI的是字符串，真实的pid
UI发给search
search返回UI的Search_request_t不变
返回的Search_result_t中 info和node的pid都是我们的id编号
然后UI输出页面的时候再变成真实pid字符串
*/
#ifndef u_short
#define u_short unsigned short
#endif

#define SEARCH_SORT_REL			0
#define SEARCH_SORT_TIME		1
#define SEARCH_SORT_REPLY		2
#define SEARCH_SORT_R_TIME		3

#define SEARCH_SORT_MIN			0
#define SEARCH_SORT_MAX			5
#define SEARCH_SORT_DEF			SEARCH_SORT_REL

typedef struct tag_Search_request_t
{
	char  query[MAX_QUERY_WORD_LEN];	//检索关键词
	u_short screen_length;			    //用户页面像素宽度
	u_short screen_height;			    //用户页面像素高度
	u_short page_no;					//翻页
	u_short language;					//编码方式 参考ty_code
	u_short pid;						//版块id
	u_short field;					    //检索范围 0 标题检索； 1 作者检索
	u_short high_light; 				//是否高亮 0 不高亮； 1 高亮
	u_short rn;						    //每页返回结果数
	u_short sort;
	u_short qt;
	u_short sn;
	u_short other;					    //预留
	char  tn[MAX_TEMPLATE_NAME_LEN];    //应用模板名称
	char  pid_str[MAX_PID_STR_LEN];     //版块id
	char  other_str[MAX_OTHER_STR_LEN];   //预留
}Search_request_t;

typedef struct tag_Search_response_t
{
	Search_request_t req;				//搜索请求信息
	u_int res_num;						//返回结果条数
	u_int total_num;					//搜索到结果总数
}Search_response_t;

typedef struct tag_Search_result_t
{
	ty_title_info_t info;
	ty_title_result_node_t node;
}Search_result_t;

typedef struct {
	u_int key1;
	u_int key2;
	u_int is_del;
	u_int sort_weight;
	char abstract[32];
	char short_str[4][32];
	char long_str[4][64];
} result_node_t;

#endif
