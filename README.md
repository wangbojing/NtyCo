## NtyCo

#### coroutine
[实现原理](https://github.com/wangbojing/NtyCo/wiki/NtyCo%E7%9A%84%E5%AE%9E%E7%8E%B0)
[配套视频讲解](https://it.0voice.com/p/t_pc/goods_pc_detail/goods_detail/course_2QFAeORw45TjJA1y9tq8CmdVJTQ)

## details
#### coroutine FSM
![](http://bojing.wang/wp-content/uploads/2018/08/status_machine.png)

#### storage structure (ready, wait, sleep, status)
![](http://bojing.wang/wp-content/uploads/2018/08/6.1.png)

#### nty_server process
![](https://cos.0voice.com/nty_server_uml.png)

#### compile

```
编译ntyco的core文件与编译libntyco.a的静态库
$ make

// 编译sample
$ make bin
```

#### err info
```
nty_mysql_oper.c:8:19: fatal error: mysql.h: No such file or directory

解决方案：
# sudo apt-get install libmysqlclient-dev

nty_rediscli.c:11:21: fatal error: hiredis.h: No such file or directory

解决方案：
需要编译安装hiredis: https://github.com/redis/hiredis

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

##### [对应视频讲解](https://ke.qq.com/course/2705727?tuin=1bf84273)
