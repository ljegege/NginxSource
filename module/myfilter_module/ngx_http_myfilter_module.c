#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
//#include <../../software/nginx-1.4.2/src/core/ngx_config.h>
//#include <../../software/nginx-1.4.2/src/core/ngx_core.h>
//#include <../../software/nginx-1.4.2/src/core/ngx_http.h>
static ngx_int_t ngx_http_myfilter_init(ngx_conf_t *cf);
static void* ngx_http_myfilter_create_conf(ngx_conf_t *cf);
//static char* ngx_http_myfilter_merge_conf(ngx_conf_t *cf, void* parent, void* child);
static ngx_int_t ngx_http_myfilter_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_myfilter_header_filter(ngx_http_request_t *r);
static ngx_int_t ngx_http_myfilter_body_filter(ngx_http_request_t *r, ngx_chain_t *in);

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;


typedef struct{
    ngx_flag_t enable;
}ngx_http_myfilter_conf_t;

typedef struct{
    ngx_int_t add_prefix;
}ngx_http_myfilter_ctx_t;

static ngx_str_t filter_prefix = ngx_string("[my filter prefix]");

static ngx_command_t ngx_http_myfilter_commands[] = {
    {ngx_string("add_prefix"),
    NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_FLAG,
    ngx_conf_set_flag_slot,
    NGX_HTTP_LOC_CONF_OFFSET,
    offsetof(ngx_http_myfilter_conf_t, enable),
    NULL},
    ngx_null_command
};

static ngx_http_module_t ngx_http_myfilter_module_ctx = {
	NULL, ngx_http_myfilter_init, NULL, NULL, NULL, NULL,
	ngx_http_myfilter_create_conf, NULL
};

ngx_module_t ngx_http_myfilter_module = {
	NGX_MODULE_V1,
	&ngx_http_myfilter_module_ctx,
	ngx_http_myfilter_commands,
	NGX_HTTP_MODULE,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_myfilter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_next_body_filter = ngx_http_top_body_filter;

    ngx_http_top_header_filter = ngx_http_myfilter_header_filter;
    ngx_http_top_body_filter = ngx_http_myfilter_body_filter;
    printf("ngx_http_myfilter_init\n");
    return NGX_OK;
}

static void* ngx_http_myfilter_create_conf(ngx_conf_t *cf)
{
    ngx_http_myfilter_conf_t *mycf;
    mycf = (ngx_http_myfilter_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_myfilter_conf_t));
    mycf->enable = NGX_CONF_UNSET;
    return mycf;
}

//static char* ngx_http_myfilter_merge_conf(ngx_conf_t *cf, void* parent, void* child)
//{
//    ngx_http_myfilter_conf_t *prev = (ngx_http_myfilter_conf_t *)parent;
//    ngx_http_myfilter_conf_t *conf = (ngx_http_myfilter_conf_t *)child;
//    ngx_conf_merge_value(conf->enable, prev->enable, 0);
//    printf("%d\n", conf->enable);
//    return NGX_CONF_OK;
//}

static ngx_int_t ngx_http_myfilter_header_filter(ngx_http_request_t *r)
{
    printf("ngx_http_myfilter_header_filter\n");
    ngx_http_myfilter_ctx_t *ctx;
    ngx_http_myfilter_conf_t *conf;

    if(r->headers_out.status != NGX_HTTP_OK){
        //return ngx_http_next_header_filter(r);
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_myfilter_module);
    if(ctx){
        return ngx_http_next_header_filter(r);
    }

    conf = ngx_http_get_module_loc_conf(r, ngx_http_myfilter_module);
    printf("header_filter:%d\n", conf->enable);
    if(conf->enable == 0){
        //return ngx_http_next_header_filter(r);
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_myfilter_ctx_t));

    ctx->add_prefix = 0;
    ngx_http_set_ctx(r, ctx, ngx_http_myfilter_module);
    if(ngx_strncasecmp(r->headers_out.content_type.data, (u_char*)"text/html", sizeof("text/html") - 1 ) == 0){
           printf("set add_prefix\n");
        ctx->add_prefix = 1;
        if(r->headers_out.content_length_n > 0){
            r->headers_out.content_length_n += filter_prefix.len;
        }
    }
    printf("can't set add_prefix\n");
    return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_myfilter_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    printf("ngx_http_myfilter_body_filter\n");
    ngx_http_myfilter_ctx_t *ctx;
    ctx = ngx_http_get_module_ctx(r, ngx_http_myfilter_module);
    if(ctx == NULL || ctx->add_prefix != 1){
        return ngx_http_next_body_filter(r, in);
    }
    ctx->add_prefix = 2;
    ngx_buf_t *b = ngx_create_temp_buf(r->pool, filter_prefix.len);
    b->start = b->pos = filter_prefix.data;
    b->last = b->pos + filter_prefix.len;
    ngx_chain_t *cl = ngx_alloc_chain_link(r->pool);
    cl->buf = b;
    cl->next = in;
    printf("%*s\n", filter_prefix.len, b->start);
    return ngx_http_next_body_filter(r, cl);
}


















