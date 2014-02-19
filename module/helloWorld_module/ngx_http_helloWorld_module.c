#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
//#include <ngx_http_proxy_module.c>
typedef struct{
	ngx_http_upstream_conf_t upstream;
}ngx_http_helloWorld_conf_t;

typedef struct{
	ngx_http_status_t status;
	ngx_str_t backendServer;
}ngx_http_helloWorld_ctx_t;

/*static ngx_str_t  ngx_http_proxy_hide_headers[] = {
     ngx_string("Date"),
     ngx_string("Server"),
     ngx_string("X-Pad"),
     ngx_string("X-Accel-Expires"),
     ngx_string("X-Accel-Redirect"),
     ngx_string("X-Accel-Limit-Rate"),
     ngx_string("X-Accel-Buffering"),
     ngx_string("X-Accel-Charset"),
     ngx_null_string
};*/

// 函数声明
static char* ngx_http_helloWorld(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void* ngx_http_helloWorld_create_loc_conf(ngx_conf_t *cf);
//static char *ngx_http_helloWorld_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t helloWorld_upstream_create_request(ngx_http_request_t *r);
static ngx_int_t helloWorld_upstream_process_header(ngx_http_request_t *r);
static void helloWorld_upstream_finalize_request(ngx_http_request_t *r, ngx_int_t rc);
static ngx_int_t helloWorld_process_status_line(ngx_http_request_t *r);
static ngx_int_t ngx_http_helloWorld_handler2(ngx_http_request_t *r);
static ngx_int_t ngx_http_helloWorld_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_mytset_handler(ngx_http_request_t *r);
static char* ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);


// 创建配置项的存储结构
static void* ngx_http_helloWorld_create_loc_conf(ngx_conf_t *cf)
{
	printf("enter ngx_http_helloWorld_create_loc_conf\n");
	ngx_http_helloWorld_conf_t *mycf;
	mycf = (ngx_http_helloWorld_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_helloWorld_conf_t));
	if(mycf == NULL){
		return NULL;
	}
	mycf->upstream.connect_timeout = 60000;
	mycf->upstream.send_timeout = 60000;
	mycf->upstream.read_timeout = 60000;
	mycf->upstream.store_access = 0600; // 用户的读权限

	mycf->upstream.buffering = 0;
	mycf->upstream.bufs.num = 8;
	mycf->upstream.bufs.size = ngx_pagesize;
	mycf->upstream.buffer_size = ngx_pagesize;
	mycf->upstream.busy_buffers_size = 2 * ngx_pagesize;
	mycf->upstream.temp_file_write_size = 2 * ngx_pagesize;
 	mycf->upstream.max_temp_file_size = 2 * ngx_pagesize;
 	mycf->upstream.hide_headers = NGX_CONF_UNSET_PTR;
 	mycf->upstream.pass_headers = NGX_CONF_UNSET_PTR;
	printf("leave ngx_http_helloWorld_create_loc_conf\n");
	return mycf;
}

/*static char *ngx_http_helloWorld_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	printf("enter ngx_http_helloWorld_merge_loc_conf\n");
	ngx_http_helloWorld_conf_t *prev = (ngx_http_helloWorld_conf_t*)parent;
	ngx_http_helloWorld_conf_t *conf = (ngx_http_helloWorld_conf_t*)child;
	ngx_hash_init_t hash;
	hash.max_size = 100;
	hash.bucket_size = 1024;
	hash.name = "proxy_headers_hash";
	if(ngx_http_upstream_hide_headers_hash(cf, &conf->upstream, &prev->upstream, ngx_http_proxy_hide_headers, &hash) != NGX_OK){
		return NGX_CONF_ERROR;
	}
	printf("leave ngx_http_helloWorld_merge_loc_conf\n");
	return NGX_CONF_OK;
}*/

static ngx_http_module_t ngx_http_helloWorld_module_ctx = {
	NULL, NULL, NULL, NULL, NULL, NULL, ngx_http_helloWorld_create_loc_conf, NULL
};

static ngx_command_t ngx_http_helloWorld_commands[] = {
	{ ngx_string("helloWorld"),
	  NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_http_helloWorld,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  0,
	  NULL },
	{ ngx_string("mytest"),
	  NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_http_mytest,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  0,
	  NULL },
	  ngx_null_command
};

ngx_module_t ngx_http_helloWorld_module = {
	NGX_MODULE_V1,
	&ngx_http_helloWorld_module_ctx,
	ngx_http_helloWorld_commands,
	NGX_HTTP_MODULE,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NGX_MODULE_V1_PADDING
};


static ngx_int_t helloWorld_upstream_create_request(ngx_http_request_t *r)
{
	printf("enter helloWorld_upstream_create_request\n");
	static ngx_str_t backendQueryLine = ngx_string("GET /search?q=%V HTTP/1.1\r\nHost: www.google.com.hk\r\nConnection: close\r\n\r\n");
	ngx_str_t args = ngx_string("lumia");
	ngx_int_t queryLineLen = backendQueryLine.len + args.len - 2;
	ngx_buf_t *b = ngx_create_temp_buf(r->pool, queryLineLen);
	if(b == NULL){
		return NGX_ERROR;
	}
	b->last = b->pos + queryLineLen;

	ngx_snprintf(b->pos, (size_t)queryLineLen, (char*)backendQueryLine.data, &args);
	r->upstream->request_bufs = ngx_alloc_chain_link(r->pool);
	r->upstream->request_bufs->buf = b;
	r->upstream->request_bufs->next = NULL;
	r->upstream->request_sent = 0;
	r->upstream->header_sent = 0;
	r->header_hash = 1;	
	printf("leave helloWorld_upstream_create_request\n");
	return NGX_OK;

}

static ngx_int_t helloWorld_upstream_process_header(ngx_http_request_t *r)
{
	printf("enter helloWorld_upstream_process_header\n");
	ngx_int_t rc;
	ngx_table_elt_t *h;
	ngx_http_upstream_header_t *hh;
	ngx_http_upstream_main_conf_t *umcf;
	umcf = ngx_http_get_module_main_conf(r, ngx_http_upstream_module);
	for(;;){
		rc = ngx_http_parse_header_line(r, &r->upstream->buffer, 1);
		if(rc == NGX_OK){
			h = ngx_list_push(&r->upstream->headers_in.headers);
			if(h == NULL){
				return NGX_ERROR;
			}			
			h->hash = r->header_hash;
			h->key.len = r->header_name_end - r->header_name_start;
			h->value.len = r->header_end - r->header_start;
			
			h->key.data = ngx_pnalloc(r->pool, h->key.len + 1 + h->value.len + 1 + h->key.len);
			if(h->key.data == NULL){
				return NGX_ERROR;
			}
			
			h->value.data = h->key.data + h->key.len + 1;
			h->lowcase_key = h->key.data + h->key.len + 1 + h->value.len + 1;
			
			ngx_memcpy(h->key.data, r->header_name_start, h->key.len);
			h->key.data[h->key.len] = '\0';
			ngx_memcpy(h->value.data, r->header_start, h->value.len);
			h->value.data[h->value.len] = '\0';
			
			ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
			hh = ngx_hash_find(&umcf->headers_in_hash, h->hash, h->lowcase_key, h->key.len);
			if(hh && hh->handler(r, h, hh->offset) != NGX_OK){
				printf("leave helloWorld_upstream_process_header 1\n");
				return NGX_ERROR;
			}
			continue;
		}
		
		if(rc == NGX_HTTP_PARSE_HEADER_DONE){
			if(r->upstream->headers_in.server == NULL){
				h = ngx_list_push(&r->upstream->headers_in.headers);
				if(h == NULL){
					return NGX_ERROR;
				}
				h->hash = ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash('s', 'e'), 'r'), 'v'), 'e'), 'r');
				ngx_str_set(&h->key, "Server");
				ngx_str_null(&h->value);
				h->lowcase_key = (u_char*) "server";
			}
			if(r->upstream->headers_in.date == NULL){
				h = ngx_list_push(&r->upstream->headers_in.headers);
				if(h == NULL){
					return NGX_ERROR;
				}
				h->hash = ngx_hash(ngx_hash(ngx_hash('d', 'a'), 't'), 'e');
				ngx_str_set(&h->key, "Date");
				ngx_str_null(&h->value);
				h->lowcase_key = (u_char*) "date";
			}
			printf("leave helloWorld_upstream_process_header 2\n");
			return NGX_OK;
		}
		
		if(rc == NGX_AGAIN){
			printf("leave helloWorld_upstream_process_header 3\n");
			return rc;
		}
		
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "upstream sent invalid header");
		printf("leave helloWorld_upstream_process_header 4\n");
		return NGX_HTTP_UPSTREAM_INVALID_HEADER;

	}
}


static void helloWorld_upstream_finalize_request(ngx_http_request_t *r, ngx_int_t rc)
{
	printf("enter helloWorld_upstream_finalize_request\n");
	ngx_log_error(NGX_LOG_DEBUG, r->connection->log, 0, "helloWorld_upstream_finalize_request");
	printf("leave helloWorld_upstream_finalize_request\n");
}

static ngx_int_t helloWorld_process_status_line(ngx_http_request_t *r)
{
	printf("enter helloWorld_process_status_line\n");
	size_t len;
	ngx_int_t rc;
	ngx_http_upstream_t *u;
	ngx_http_helloWorld_ctx_t *ctx = ngx_http_get_module_ctx(r, ngx_http_helloWorld_module);
	if(ctx == NULL){
		printf("can get the nginx context!\n");
		return NGX_ERROR;
	}
	u = r->upstream;
	rc = ngx_http_parse_status_line(r, &u->buffer, &ctx->status);
	if(rc == NGX_AGAIN){
		return rc;
	}
	if(rc == NGX_ERROR){
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "upstream sent no valid http header");
		r->http_version = NGX_HTTP_VERSION_9;
		u->state->status = NGX_HTTP_OK;
		return NGX_OK;
	}
	
	//解析http 头部成功
	if(u->state){
		u->state->status = ctx->status.code;
	}
	u->headers_in.status_n = ctx->status.code;
	
	len = ctx->status.end - ctx->status.start;
	u->headers_in.status_line.data = ngx_pnalloc(r->pool, len);
	if(u->headers_in.status_line.data == NULL){
		return NGX_ERROR;
	}
	ngx_memcpy(u->headers_in.status_line.data, ctx->status.start, len);
	printf("%*s\n", len, ctx->status.start);
	u->process_header = helloWorld_upstream_process_header;
	printf("leave helloWorld_process_status_line\n");
	return helloWorld_upstream_process_header(r);


}

static ngx_int_t ngx_http_helloWorld_handler2(ngx_http_request_t *r)
{
	printf("enter ngx_http_helloWorld_handler2\n");
	ngx_http_helloWorld_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_helloWorld_module);
	if(myctx == NULL){
		myctx = ngx_palloc(r->pool, sizeof(ngx_http_helloWorld_ctx_t));
		if(myctx == NULL){
			return NGX_ERROR;
		}
		ngx_http_set_ctx(r, myctx, ngx_http_helloWorld_module);
	}
	
	if(ngx_http_upstream_create(r) != NGX_OK){
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "upstream create failed!");
		return NGX_ERROR;
	}
	
	ngx_http_helloWorld_conf_t *mycf = (ngx_http_helloWorld_conf_t*) ngx_http_get_module_loc_conf(r, ngx_http_helloWorld_module);
	ngx_http_upstream_t *u = r->upstream;
	u->conf = &mycf->upstream;
	u->buffering = mycf->upstream.buffering;
	
	u->resolved = (ngx_http_upstream_resolved_t*) ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_resolved_t));
	if(u->resolved == NULL){
		return NGX_ERROR;
	}
	
	static struct sockaddr_in backendSockAddr;
	struct hostent *pHost = gethostbyname((char*)"www.google.com.hk");
	if(pHost == NULL){
		return NGX_ERROR;
	}
	
	backendSockAddr.sin_family = AF_INET;
	backendSockAddr.sin_port = htons(80);
	char* pDmsIP = inet_ntoa(*(struct in_addr*)(pHost->h_addr_list[0]));
	backendSockAddr.sin_addr.s_addr = inet_addr(pDmsIP);
	myctx->backendServer.data = (u_char*) pDmsIP;
	myctx->backendServer.len = strlen(pDmsIP);
	u->resolved->sockaddr = (struct sockaddr *) &backendSockAddr; 
	u->resolved->socklen = sizeof(struct sockaddr_in);
	u->resolved->naddrs = 1;
	
	u->create_request = helloWorld_upstream_create_request;
	u->process_header = helloWorld_process_status_line;
	u->finalize_request = helloWorld_upstream_finalize_request;
	
	r->main->count++;
	ngx_http_upstream_init(r);
	printf("leave ngx_http_helloWorld_handler2\n");
	return NGX_DONE;
}

static ngx_int_t ngx_http_helloWorld_handler(ngx_http_request_t *r)
{
	printf("enter ngx_http_helloWorld_handler\n");
	if(!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))){
		return NGX_HTTP_NOT_ALLOWED;
	}

	ngx_int_t rc = ngx_http_discard_request_body(r);
	if(rc != NGX_OK){
		return rc;
	}

	ngx_str_t type = ngx_string("text/plain");
	ngx_str_t response = ngx_string("lje hello world");
	if(r->headers_in.content_type){
		printf("found content type\n");
	}else{
		printf("not found content type\n");
	}
	printf("%*s\n", r->headers_in.server.len, r->headers_in.server.data);
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_length_n = response.len;
	r->headers_out.content_type = type;
	
	// 发送http头部
	rc = ngx_http_send_header(r);
	if(rc == NGX_ERROR || rc > NGX_OK || r->header_only){
		return rc;
	}

	ngx_buf_t *b;
	b = ngx_create_temp_buf(r->pool, response.len);
	if(b == NULL){
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}

	ngx_memcpy(b->pos, response.data, response.len);
	b->last = b->pos + response.len;
	b->last_buf = 1;
	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;
	printf("leave ngx_http_helloWorld_handler\n");
	return ngx_http_output_filter(r, &out);
}

static ngx_int_t ngx_http_mytset_handler(ngx_http_request_t *r)
{
	if(!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD))){
		return NGX_HTTP_NOT_ALLOWED;
	}

	ngx_int_t rc = ngx_http_discard_request_body(r);
	if(rc != NGX_OK){
		return rc;
	}

	ngx_buf_t *b;
	b = ngx_palloc(r->pool, sizeof(ngx_buf_t));
	b->in_file = 1;
	b->file = ngx_palloc(r->pool, sizeof(ngx_file_t));
	b->file->fd = ngx_open_file("/home/lje/test.txt", NGX_FILE_RDONLY|NGX_FILE_NONBLOCK, NGX_FILE_OPEN, 0);
	b->file->log = r->connection->log;
	b->file->name.data = (u_char*)"/home/lje/test.txt";
	b->file->name.len = sizeof("/home/lje/test.txt") - 1;
	if(b->file->fd <= 0){
		printf("can't open the file\n");
		return NGX_HTTP_NOT_FOUND;
	}

	if(ngx_file_info("/home/lje/test.txt", &b->file->info) == NGX_FILE_ERROR){
		printf("can't get the file's infomation\n");
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	
	ngx_str_t type = ngx_string("text/html");
	r->headers_out.status = NGX_HTTP_OK;
	r->headers_out.content_type = type;
	r->headers_out.content_length_n = b->file->info.st_size;
	
	b->file_pos = 0;
	b->file_last = b->file->info.st_size;

	// 发送http头部
	rc = ngx_http_send_header(r);
	if(rc == NGX_ERROR || rc > NGX_OK || r->header_only){
		printf("can't send the http header\n");
		return rc;
	}
	printf("send the http header success\n");
	ngx_chain_t out;
	out.buf = b;
	out.next = NULL;
	return ngx_http_output_filter(r, &out);
}

static char* ngx_http_helloWorld(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	printf("enter ngx_http_helloWorld\n");
	ngx_http_core_loc_conf_t *clcf;
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_helloWorld_handler;
	clcf->handler = ngx_http_helloWorld_handler2;
	printf("leave ngx_http_helloWorld\n");
	return NGX_CONF_OK;
}

static char* ngx_http_mytest(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	printf("enter ngx_http_mytest\n");
	ngx_http_core_loc_conf_t *clcf;
	clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_mytset_handler;
	printf("leave ngx_http_mytest\n");
	return NGX_CONF_OK;
}

