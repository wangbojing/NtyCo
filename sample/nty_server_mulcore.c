/*
 *  Author : WangBoJing , email : 1989wangbojing@gmail.com
 * 
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Author. (C) 2017
 * 
 *

****       *****                                      *****
  ***        *                                       **    ***
  ***        *         *                            *       **
  * **       *         *                           **        **
  * **       *         *                          **          *
  *  **      *        **                          **          *
  *  **      *       ***                          **
  *   **     *    ***********    *****    *****  **                   ****
  *   **     *        **           **      **    **                 **    **
  *    **    *        **           **      *     **                 *      **
  *    **    *        **            *      *     **                **      **
  *     **   *        **            **     *     **                *        **
  *     **   *        **             *    *      **               **        **
  *      **  *        **             **   *      **               **        **
  *      **  *        **             **   *      **               **        **
  *       ** *        **              *  *       **               **        **
  *       ** *        **              ** *        **          *   **        **
  *        ***        **               * *        **          *   **        **
  *        ***        **     *         **          *         *     **      **
  *         **        **     *         **          **       *      **      **
  *         **         **   *          *            **     *        **    **
*****        *          ****           *              *****           ****
                                       *
                                      *
                                  *****
                                  ****



 *
 */




#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


#define __USE_GNU

#include <sched.h>
#include <string.h>
#include <pthread.h>

#include <ctype.h>

#include "nty_coroutine.h"

static int global_shmid = -1;
static int global_semid = -1;

#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)


typedef struct _shm_area {
	int total;
	int lock;
	struct timeval tv_begin;
} shm_area;

static shm_area *global_shmaddr = NULL;


#define MAX_CLIENT_NUM			1000000


struct timeval* get_shm_timevalue(void);
void set_shm_timevalue(struct timeval *tv);


int add_shmvalue(void);
int sub_shmvalue(void);

int init_shmvalue(void);


unsigned long cmpxchg(void *addr, unsigned long _old, unsigned long _new, int size) {
	unsigned long prev;
	volatile unsigned int *_ptr = (volatile unsigned int *)(addr);

	switch (size) {
		case 1: {
			__asm__ volatile (
		        "lock; cmpxchgb %b1, %2"
		        : "=a" (prev)
		        : "r" (_new), "m" (*_ptr), "0" (_old)
		        : "memory");
			break;
		}
		case 2: {
			__asm__ volatile (
		        "lock; cmpxchgw %w1, %2"
		        : "=a" (prev)
		        : "r" (_new), "m" (*_ptr), "0" (_old)
		        : "memory");
			break;
		}
		case 4: {
			__asm__ volatile (
		        "lock; cmpxchg %1, %2"
		        : "=a" (prev)
		        : "r" (_new), "m" (*_ptr), "0" (_old)
		        : "memory");
			break;
		}
	}

	return prev;
}

#define SERVER_STRING "Server: ntyco\r\n"


int headers(char *buffer)
{
    strcat(buffer, "HTTP/1.0 200 OK\r\n");
    strcat(buffer, SERVER_STRING);
    strcat(buffer, "Content-Type: text/html\r\n");
    strcat(buffer, "\r\n");

	return strlen(buffer);
}

int body(char *buffer) {
	strcat(buffer, "<html><h1> coroutine --> ntyco<h1></html>");

	return strlen(buffer);
}

void server_reader(void *arg) {
	int fd = *(int *)arg;
	int ret = 0;


	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLLIN;

	while (1) {
		
		char buf[1024] = {0};
		ret = nty_recv(fd, buf, 1024, 0);
		if (ret > 0) {
			if(fd > MAX_CLIENT_NUM) 
				printf("read from server: %.*s\n", ret, buf);

			memset(buf, 0, 1024);
			int hlen = headers(buf);
			int blen = body(buf+hlen);

			ret = nty_send(fd, buf, strlen(buf), 0);
			if (ret == -1) {
				nty_close(fd);
				sub_shmvalue();
				
				break;
			}
		} else if (ret == 0) {	
			nty_close(fd);
			
			sub_shmvalue();
			
			break;
		}
	}
}


void server(void *arg) {

	unsigned short port = *(unsigned short *)arg;
	free(arg);

	int fd = nty_socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return ;

	struct sockaddr_in local, remote;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = INADDR_ANY;
	bind(fd, (struct sockaddr*)&local, sizeof(struct sockaddr_in));

	listen(fd, 20);
	printf("listen port : %d\n", port);

	while (1) {
		socklen_t len = sizeof(struct sockaddr_in);
		int cli_fd = nty_accept(fd, (struct sockaddr*)&remote, &len);

		printf("new client comming\n");
		
		nty_coroutine *read_co;
		nty_coroutine_create(&read_co, server_reader, &cli_fd);

		int value = add_shmvalue();
		if (value % 1000 == 999) {
			struct timeval *tv_begin = get_shm_timevalue();

			struct timeval tv_cur;
			memcpy(&tv_cur, tv_begin, sizeof(struct timeval));	
			gettimeofday(tv_begin, NULL);

			int time_used = TIME_SUB_MS((*tv_begin), tv_cur);
			
			printf("client sum: %d, time_used: %d\n", value, time_used);
		}

	}
	
}

int mulcore_entry(int begin_port) {

	nty_coroutine *co = NULL;

	int i = 0;
	unsigned short base_port = begin_port;
	//for (i = 0;i < 10;i ++) {
	unsigned short *port = calloc(1, sizeof(unsigned short));
	*port = base_port + i;
	nty_coroutine_create(&co, server, port); ////////no run
	//}

	nty_schedule_run(); //run

}

int process_bind(void) {

	int num = sysconf(_SC_NPROCESSORS_CONF);

	pid_t self_id = syscall(__NR_gettid);
	//printf("selfid --> %d\n", self_id);

	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(self_id % num, &mask);

	sched_setaffinity(0, sizeof(mask), &mask);

	mulcore_entry(9096);

}


struct timeval* get_shm_timevalue(void) {

	return global_shmaddr ? &global_shmaddr->tv_begin : NULL;

}

void set_shm_timevalue(struct timeval *tv) {
	if (global_shmaddr == NULL || tv == NULL) {
		return ;
	}

	memcpy(&global_shmaddr->tv_begin, tv, sizeof(struct timeval));
	
}

int add_shmvalue(void) {
	int value = 0;
	int ret = -1;

	do {

		value = global_shmaddr->total;
		ret = cmpxchg(&global_shmaddr->total, (unsigned long)value, (unsigned long)(value+1), 4);

	} while (ret != value);

	return global_shmaddr->total;
}

int sub_shmvalue(void) {
	int value = 0;
	int ret = -1;

	do {

		value = global_shmaddr->total;
		ret = cmpxchg(&global_shmaddr->total, (unsigned long)value, (unsigned long)(value-1), 4);

	} while (ret != value);

	return global_shmaddr->total;
}


int init_shmvalue(void) {
	
	global_shmid = shmget(IPC_PRIVATE, sizeof(shm_area), IPC_CREAT|0600);
	if (global_shmid < 0) {
		perror("shmget failed\n");
		return -1;
	}
	global_shmaddr = (shm_area*)shmat(global_shmid, NULL, 0);
	if (global_shmaddr == (shm_area*)-1) {
		perror("shmat addr error");
		return -1;
	}
	global_shmaddr->total = 0;
	gettimeofday(&global_shmaddr->tv_begin, NULL);

}



int main(int argc, char *argv[]) {

	int ret = init_shmvalue();
	if (ret == -1) {
		printf("init share memory failed\n");
		return -1;
	}
	
	fork();
	fork();
	//fork();
	//fork();
	//fork();

	process_bind();

}

