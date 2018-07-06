
CC = gcc
BIN = nty_server_test
FLAG = -lpthread

OBJS = *.o


all : 
	$(CC) -c nty_*.c
	$(CC) -o $(BIN) *.o $(FLAG) 
clean :
	rm -rf *.o $(BIN)

