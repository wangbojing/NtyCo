
CC = gcc

BIN = nty_server nty_server_mulcore nty_http_server nty_websocket_server
FLAG = -lpthread -O3 -lcrypto -lssl

all : $(BIN)
.PHONY : all

nty_server : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_server.o
	$(CC) -o $@ $^ $(FLAG)

nty_server_mulcore : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_server_mulcore.o
	$(CC) -o $@ $^ $(FLAG)

nty_http_server : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_http_server.o
	$(CC) -o $@ $^ $(FLAG)

nty_websocket_server : nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o nty_websocket_server.o
	$(CC) -o $@ $^ $(FLAG)

nty_socket.o : nty_socket.c
	$(CC) -c $<

nty_coroutine.o : nty_coroutine.c
	$(CC) -c $<

nty_epoll.o : nty_epoll.c
	$(CC) -c $<

nty_schedule.o : nty_schedule.c
	$(CC) -c $<

nty_server.o : nty_server.c
	$(CC) -c $<

nty_server_mulcore.o : nty_server_mulcore.c
	$(CC) -c $<

nty_http_server.o : nty_http_server.c
	$(CC) -c $<

clean :
	rm -rf *.o $(BIN)

