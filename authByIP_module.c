
/*
 * Copyright (C) Nginx, Inc.
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_list_t *list;
} ngx_http_authByIP_main_conf_t;

static ngx_int_t ngx_http_authByIP_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_authByIP_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_variable_authByIP_get(ngx_http_request_t *r,
                                                ngx_http_variable_value_t *v,
                                                uintptr_t data);

static void *ngx_http_authByIP_create_main_conf(ngx_conf_t *cf);

static ngx_http_module_t ngx_http_authByIP_module_ctx = {
    ngx_http_authByIP_init, /* preconfiguration */
    NULL,                   /* postconfiguration */

    ngx_http_authByIP_create_main_conf, /* create main configuration */
    NULL,                               /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL  /* merge location configuration */
};

// clang-format off
ngx_module_t ngx_http_authByIP_module = {
    NGX_MODULE_V1,
    &ngx_http_authByIP_module_ctx, /* module context */
    NULL,                          /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};
// clang-format on

static void *ngx_http_authByIP_create_main_conf(ngx_conf_t *cf) {
    ngx_http_authByIP_main_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_authByIP_main_conf_t));
    if (conf == NULL) { /* error */
        return NULL;
    }

    conf->list = ngx_list_create(cf->pool, 100, sizeof(ngx_str_t));
    if (conf->list == NULL) { /* error */
        return NULL;
    }

    return conf;
}

static ngx_int_t ngx_http_authByIP_init(ngx_conf_t *cf) {

    ngx_http_authByIP_add_variables(cf);

    return NGX_OK;
}

ngx_int_t ngx_http_authByIP_subrequest_done(ngx_http_request_t *r, void *data,
                                            ngx_int_t rc) {
    // 获取父请求
    ngx_http_request_t *pr = r->parent;

    // 获取父请求 authByIP_module 上下文
    ngx_http_authByIP_main_conf_t *flcf;
    flcf = ngx_http_get_module_main_conf(pr, ngx_http_authByIP_module);

    // 查看返回码，如果为NGX_HTTP_OK，则意味访问成功
    if (r->headers_out.status == NGX_HTTP_OK) {
        // 将父请求的IP地址加入链表
        ngx_str_t *hs = &pr->headers_in.host->value;

        ngx_str_t *node = ngx_list_push(flcf->list);
        if (node == NULL) { /* error */
            return NGX_ERROR;
        }
        ngx_str_set(node, hs->data);

        // ngx_int_t* f = data;
        // *f = 1;
    }

    return NGX_OK;
}

ngx_int_t ngx_http_authByIP_subrequest_start(ngx_http_request_t *r) {
    ngx_str_t uri;
    ngx_str_t args = ngx_string("ip=127.0.0.1");
    ngx_http_request_t *sr;
    ngx_http_post_subrequest_t *ps;


    // ngx_int_t* flag = ngx_palloc(r->pool, sizeof(ngx_int_t));
    // *flag = 0;

    ngx_str_set(&uri, "/foo");
    // ngx_str_set(&uri, "/eye");

    ps = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
    if (ps == NULL) {
        return NGX_ERROR;
    }
    ps->handler = ngx_http_authByIP_subrequest_done;
    // ps->data = flag;
    
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "parent request uri: %V, args: %V", &r->uri, &r->args);
    
    ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "sub request uri: %V, args: %V", &uri, &args);

    ngx_int_t rc = ngx_http_subrequest(r, &uri, &args, &sr, ps,
                                       NGX_HTTP_SUBREQUEST_IN_MEMORY);
    
    // ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0, "f: %i", *flag);

    
    if (rc == NGX_ERROR) { /* error */
        return NGX_ERROR;
    }
    return NGX_DONE;
}

static ngx_int_t ngx_http_variable_authByIP_get(ngx_http_request_t *r,
                                                ngx_http_variable_value_t *v,
                                                uintptr_t data) {
    // get the host
    ngx_str_t *hs;
    hs = &r->headers_in.host->value;

    // get authByIP main conf
    ngx_http_authByIP_main_conf_t *flcf;
    flcf = ngx_http_get_module_main_conf(r, ngx_http_authByIP_module);

    /* flag */
    ngx_uint_t f = 0;

    /* iterate over the list */
    ngx_str_t *vs;
    ngx_list_part_t *part;
    ngx_uint_t i;

    part = &flcf->list->part;
    vs = part->elts;

    for (i = 0; /* void */; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            vs = part->elts;
            i = 0;
        }

        /* ngx_do_smth(&vs[i]); */
        if (ngx_strcmp(vs[i].data, hs->data) == 0) {
            f = 1;
            break;
        }
    }

    if (f == 0) {
        /* Whether to join according to the conditions */

        /* insert node */
        // ngx_str_t *node = ngx_list_push(flcf->list);
        // if (node == NULL) { /* error */
        //     return NGX_ERROR;
        // }
        // ngx_str_set(node, hs->data);

        // sent a subrequest
        ngx_http_authByIP_subrequest_start(r);
    }

    /* set the $authByIP variable */
    ngx_str_t *s;
    s = ngx_pnalloc(r->pool, sizeof(ngx_str_t));
    if (s == NULL) { /* error */
        return NGX_ERROR;
    }

    if (f == 1) {
        ngx_str_set(s, "true");
    } else {
        ngx_str_set(s, "false");
    }

    // test
    // /* false with "host: 127.0.0.1" */
    // if (ngx_strcmp(hs->data, "127.0.0.1") == 0) {
    //     ngx_str_set(s, "false");
    // } else {
    //     ngx_str_set(s, "true");
    // }

    v->data = s->data;
    v->len = s->len;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;
    v->escape = 0;

    return NGX_OK;
}

// clang-format off
static ngx_http_variable_t ngx_http_foo_vars[] = {

    {ngx_string("authByIP"), NULL, ngx_http_variable_authByIP_get, 0, 0, 0},

    ngx_http_null_variable
};
// clang-format on

static ngx_int_t ngx_http_authByIP_add_variables(ngx_conf_t *cf) {
    ngx_http_variable_t *var, *v;

    for (v = ngx_http_foo_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }
        var->set_handler = v->set_handler;
        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}