## NtyCo

#### coroutine
e-book：[NtyCo实现原理](http://ntycobook.ntytcp.com/)

## [目录](http://ntycobook.ntytcp.com/index.html)
* [前言](http://ntycobook.ntytcp.com/charpter0/index.html)
* [第一章 协程的起源](http://ntycobook.ntytcp.com/chapter1/1_0.html)
* [第二章 协程的案例](http://ntycobook.ntytcp.com/index.html)
* [第三章 协程的实现之工作原理](http://ntycobook.ntytcp.com/index.html)
    * [创建协程](http://ntycobook.ntytcp.com/index.html)
    * [实现IO异步操作](http://ntycobook.ntytcp.com/index.html)
    * [回调协程的子操作](http://ntycobook.ntytcp.com/index.html)
* [第四章 协程的实现之原语操作](http://ntycobook.ntytcp.com/index.html)
* [第五章 协程实现之切换](http://ntycobook.ntytcp.com/index.html)
* [第六章 协程实现之定义](http://ntycobook.ntytcp.com/index.html)
    * [运行体高效切换](http://ntycobook.ntytcp.com/index.html)
    * [调度器与协程的功能界限](http://ntycobook.ntytcp.com/index.html)
* [第七章 协程实现之调度器](http://ntycobook.ntytcp.com/index.html)
    * [生产者消费者模式](http://ntycobook.ntytcp.com/index.html)
    * [多状态下运行](http://ntycobook.ntytcp.com/index.html)
* [第八章 协程性能测试](http://ntycobook.ntytcp.com/index.html)
* [第九章 协程多核模式](http://ntycobook.ntytcp.com/index.html)

## details
#### coroutine FSM
![](http://bojing.wang/wp-content/uploads/2018/08/status_machine.png)

#### storage structure (ready, wait, sleep, status)
![](http://bojing.wang/wp-content/uploads/2018/08/6.1.png)


#### compile

```
$ make
```


#### server 
```
$ ./bin/nty_server
```
#### client
```
./bin/nty_client
```

#### mul_process, mul_core
```
$ ./bin/nty_server_mulcore
```
#### websocket
```
$ ./bin/nty_websocket_server
```

#### bench
```
$ ./bin/nty_bench
```
![](http://bojing.wang/wp-content/uploads/2018/08/nty_bench_ntyco.png)
![](http://bojing.wang/wp-content/uploads/2018/08/nty_bench_nginx.png)


#### http server
```
$ ./bin/nty_http_server_mulcore
```

![](http://bojing.wang/wp-content/uploads/2018/08/ntyco_ab.png)![](http://bojing.wang/wp-content/uploads/2018/08/nginx_ab.png)

##  鸣谢
在此要特别感谢我们的团队的每一位成员的努力，也特别感谢背后默默支持我们的家人们。如果你有任何疑问，或者想和我们探讨技术请联系我们：

email: 1989wangbojing@163.com  
email: lizhiyong4360@gmail.com  
email: 592407834@qq.com  
qq群: 829348971
