# network_prjct
cqu, 2020, fall, Computer Networks
李颀琳(20185653)

### 一、文件目录
```
---- network_proj
	|------ mac.c
	|------ mac.h 数据链路层头文件
	|------ main 主函数执行文件
	|------ network.c 网络层文件
	|------ network.h
	|------ test.c 测试一些string操作的代码 可忽略
 	|------ transport.c
 	|------ transport.h
```

### 二、修改历史
2020.12.25 
1. 实现功能：从udp加上头部后发送到ip层，再有ip层发送到数据链路层（文件形式）；接收到文件后，数据链路层将帧拆包，取出payload发送给ip层，ip层进行头部检验以后把数据报发送给udp。

2020.12.26
1. 主要任务：测试并实现分片的任务

2020.12.30
1.再次改进之前的的分片方案，上传最终版本
