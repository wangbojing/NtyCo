
CC = gcc
BINDIR = bin/
BIN = nty_server nty_server_mulcore nty_http_server nty_websocket_server nty_http_server_mulcore
FLAG = -lpthread -O3 -lcrypto -lssl

all : $(BIN)
.PHONY : all

nty_server : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_server.o
	$(CC) -o $(BINDIR)$@ $^ $(FLAG)

nty_server_mulcore : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_server_mulcore.o
	$(CC) -o $(BINDIR)$@ $^ $(FLAG)

nty_http_server : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_http_server.o
	$(CC) -o $(BINDIR)$@ $^ $(FLAG)

nty_websocket_server : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_websocket_server.o
	$(CC) -o $(BINDIR)$@ $^ $(FLAG)

nty_http_server_mulcore : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_http_server_mulcore.o
	$(CC) -o $(BINDIR)$@ $^ $(FLAG)

clean :
	rm -rf *.o $(BINDIR)$(BIN)

