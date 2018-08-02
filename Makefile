
CC = gcc

BIN = nty_server nty_server_mulcore
FLAG = -lpthread

COMMON_OBJS = nty_socket.o nty_coroutine.o nty_epoll.o nty_schedule.o


all : 
	$(CC) -c nty_*.c
	$(CC) -o nty_server nty_server.o $(COMMON_OBJS)  $(FLAG) 
	$(CC) -o nty_server_mulcore nty_server_mulcore.o $(COMMON_OBJS) $(FLAG)
clean :
	rm -rf *.o $(BIN)

