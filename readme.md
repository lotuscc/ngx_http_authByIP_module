`ngx_http_authByIP_module` 简介

## 功能

通过请求的IP地址验证请求是否合法。该模块主要导出了一个 `$authByIP` 变量，通过该变量可以判断请求的IP地址是否为允许通过的IP地址。

## 使用方式

基本的使用方式如下：
如果请求的IP地址可以通过验证，则 `$authByIP` 变量的值为`true`。否则其值为`false`。
可以根据 `$authByIP` 变量的值来分割请求，将通过验证的请求转发到正确的后端程序，将无法通过验证的请求转发到错误处理。


```nginx
if ($authByIP = "true") {                        
    # proxy_pass @app-backend
    return 200 "true";          
}
# proxy_pass @app-err
return 200 "false";
```

## 核心原理

```c
typedef struct {
    ngx_list_t *list;
} ngx_http_authByIP_main_conf_t;
```

`ngx_http_authByIP_module` 模块内部维护了一个 `list`，以及创建了一个`$authByIP` 变量。
每次获取`$authByIP`变量值的时候都会触发一个回调函数。回调函数中查询当前请求的IP地址在`list`中,如果在`list`中则说明该请求是合法的，`$authByIP` 变量的值设置为`true`。否则会向外部验证服务器发起一个子请求，根据子请求的结果来判断当前请求的IP是否能通过验证。

子请求格式为：http://www.auth.com/query?ip=127.0.0.1

其中验证服务器地址可以自己修改，IP以参数的形式

这些操作都是在回调函数中进行的，也就是说，如果不在`nginx.conf`配置文件中显示获取`$authByIP` 变量的值，这些验证都不会发生，此时请求正常进行。