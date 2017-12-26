# 服务端文档
## 入门 ﻿﻿
本项目需配合 http://tucodec.com/ 中的开发者文档使用。VoIP库、WIN/MAC/iOS/Android端demo以及文档均在图鸭科技官网的开发者板块中。服务端提供编译后可直接运行的代码，客户端部分仅提供视频通讯用SDK.

## 编译

项目结构如下：
>relay_server  
├─3rd				第三方依赖  
│  ├─curl  
│  └─libuv  
├─bin				可执行文件  
│  └─log			日志  
├─ide				Visual Studio sln文件存放位置  
├─lib				依赖库  
└─src  
    ├─clientdemo		客户端通信用demo  
    ├─client_network		客户端网络通信模块  
    ├─relay_server_main	服务器从节点可执行程序  
    ├─relay_server_master	服务器主节点可执行程序  
    ├─relay_server_sdk	服务器从节点库  
    └─simple_uv		libuv封装库  

## 编译方法：  
进入relay_server文件夹，make(需要先安装cmake)，自动编译依赖库以及服务端代码，最终在relay_server/bin文件夹生成relay_server_main(转发服务器从节点).

## 配置文件详情：
配置信息可以是乱序，每条配置按照 slave_port=12345 格式

#### 从节点配置文件名：slave.config

从节点可配置信息：

##### slave_ip=123.234.123.234

从节点tcp/udp套接字公网IP。用于向主节点汇报自己的公网IP和监听。
##### slave_port=12345

从节点tcp/udp套接字所监听端口。若不填写该配置，视为监听12345端口
http_validate=http://www.tucodec.com/relay_server/login
客户端登陆从节点时所需鉴权接口，默认采用http回调方式，返回值为0说明允许登陆。不填写该配置则允许所有登陆请求。
##### master_ip=0.0.0.0

主节点tcp套接字所监听ip。若不填写该配置，视为不存在主节点
##### master_port=12345
主节点tcp套接字所监听端口。若不填写该配置，视为不存在主节点

#### 主节点配置文件名:master.config

主节点可配置信息：

##### master_ip=0.0.0.0

主节点tcp套接字所监听IP。若不填写该配置，视为监听0.0.0.0 地址
master_port=10000
主节点tcp套接字所监听端口。若不填写该配置，视为监听10000端口
slave_node=127.0.0.1;123.234.123.234
从节点白名单,请严格按照 ip1;ip2;ip3 格式。

## 服务器安全

默认情况下，服务器配置文件中有一个配置选项“http_validate_on_login”,表示在客户端发送登陆请求时，服务器如何判断是否允许该操作。
http_validate_on_login选项应设置为一个http接口。
当http_validate_on_login= http://www.tucodec.com/relay_server/login 时，
服务器收到登陆请求时，会发送http GET请求，即向

>http://www.tucodec.com/relay_server/login?uid=xxx&sessionId=xxx

请求数据。其响应为单个字符1时，表示允许登陆，其余情况为不允许登陆。

如想要采取其他方式判断客户端登陆的合法性，可以重载relay_server.h文件的 CRelayServer类中的ValidateOnLogin函数。自行设计鉴权方法。

```
virtual bool ValidateOnLogin(uint32_t uid1, uint32_t sessionId, std::string *url);
```

## 运行
在bin目录下./relay_server_main即可运行。

## 客户端SDK下载
目前客户端SDK有安卓，iOS，Mac，Windows版本，下载地址为http://develop.tucodec.com/#/download，也可以通过官网tucodec.com链接过去。每个SDK中均附有对应的API说明。

