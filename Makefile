
CC = gcc
ECHO = echo

SUB_DIR = core/
SAMPLE_DIR = sample/
ROOT_DIR = $(shell pwd)
OBJS_DIR = $(ROOT_DIR)/objs
BIN_DIR = $(ROOT_DIR)/bin

BIN = nty_server nty_client nty_bench nty_server_mulcore hook_tcpserver nty_http_server nty_mysql_client nty_mysql_oper nty_websocket_server nty_http_server_mulcore ntyco_httpd nty_rediscli

LIB = libntyco.a

FLAG = -lpthread -O3 -ldl -I $(ROOT_DIR)/core 

THIRDFLAG = -lcrypto -lssl -lmysqlclient -lhiredis -I /usr/include/mysql/ -I /usr/local/include/hiredis/

CUR_SOURCE = ${wildcard *.c}
CUR_OBJS = ${patsubst %.c, %.o, %(CUR_SOURCE)}

export CC BIN_DIR OBJS_DIR ROOT_IDR FLAG BIN ECHO EFLAG



all : check_objs check_bin $(SUB_DIR) $(LIB)
.PHONY : all

bin: $(SAMPLE_DIR) $(BIN)

# lib: $(SUB_DIR) $(LIB)

$(SUB_DIR) : ECHO
	make -C $@

#DEBUG : ECHO
#	make -C bin

ECHO :
	@echo $(SUB_DIR)

check_objs:
	if [ ! -d "objs" ]; then \
		mkdir -p objs;  \
	fi

check_bin:
	if [ ! -d "bin" ]; then \
		mkdir -p bin;   \
	fi


nty_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_server.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

nty_client : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_client.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

nty_bench : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_bench.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

nty_server_mulcore : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_server_mulcore.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

nty_http_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_http_server.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

nty_websocket_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_websocket_server.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(THIRDFLAG)

nty_mysql_client : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_mysql_client.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(THIRDFLAG)

nty_mysql_oper : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_mysql_oper.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(THIRDFLAG)

nty_rediscli : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_rediscli.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(THIRDFLAG)

hook_tcpserver: $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/hook_tcpserver.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG)

nty_http_server_mulcore : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/nty_http_server_mulcore.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

ntyco_httpd : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(SAMPLE_DIR)/ntyco_httpd.c
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) 

libntyco.a : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o
	ar rcs $@ $^

clean :
	rm -rf $(BIN_DIR)/* $(OBJS_DIR)/* libntyco.a

