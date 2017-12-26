# relay_server_sdk  
基于simple_uv库开发的P2P服务器库，可在win/linux下编译使用  
## 部分文件说明  
    client_tcpserver_msg.h   客户端与信令服务器通信用消息定义  
    client_udpserver_msg.h   客户端与媒体服务器通信用消息定义  
    media_server.cpp         udp服务器：处理媒体数据的转发
    media_server.h            
    relay_server.cpp         udp与tcp服务器的聚合  
    relay_server.h  
    signaling_server.cpp     tcp信令服务器：处理转发表的修改请求、协调P2P
    signaling_server.h  
    slave.cpp                用于与集群主节点通信  
    slave.h                       
	

