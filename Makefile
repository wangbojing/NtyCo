
CC = gcc
ECHO = echo

SUB_DIR = core/ sample/
ROOT_DIR = $(shell pwd)
OBJS_DIR = $(ROOT_DIR)/objs
BIN_DIR = $(ROOT_DIR)/bin

BIN = nty_server nty_client nty_bench nty_server_mulcore nty_http_server nty_websocket_server nty_http_server_mulcore ntyco_httpd
FLAG = -lpthread -O3 -lcrypto -lssl -I $(ROOT_DIR)/core
EFLAG = -e ""

CUR_SOURCE = ${wildcard *.c}
CUR_OBJS = ${patsubst %.c, %.o, %(CUR_SOURCE)}

export CC BIN_DIR OBJS_DIR ROOT_IDR FLAG BIN ECHO EFLAG

all : $(SUB_DIR) $(BIN)
.PHONY : all


$(SUB_DIR) : ECHO
	make -C $@

#DEBUG : ECHO
#	make -C bin

ECHO :
	@echo $(SUB_DIR)


nty_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(OBJS_DIR)/nty_server.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

nty_client : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(OBJS_DIR)/nty_client.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

nty_bench : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(OBJS_DIR)/nty_bench.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

nty_server_mulcore : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(OBJS_DIR)/nty_server_mulcore.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

nty_http_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(OBJS_DIR)/nty_http_server.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

nty_websocket_server : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(OBJS_DIR)/nty_websocket_server.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

nty_http_server_mulcore : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(OBJS_DIR)/nty_http_server_mulcore.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

ntyco_httpd : $(OBJS_DIR)/nty_socket.o $(OBJS_DIR)/nty_coroutine.o $(OBJS_DIR)/nty_epoll.o $(OBJS_DIR)/nty_schedule.o $(OBJS_DIR)/ntyco_httpd.o
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) && $(ECHO) $(EFLAG)

clean :
	rm -rf $(BIN_DIR)/* $(OBJS_DIR)/*

