



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

static int *global_shmaddr = (int*)-1;

#define MAX_CLIENT_NUM			1000000


int get_shmvalue(void);
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



void server_reader(void *arg) {
	int fd = *(int *)arg;
	int ret = 0;


	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLLIN;

	while (1) {
		
		char buf[100] = {0};
		ret = nty_recv(fd, buf, 100);
		if (ret > 0) {
			if(fd > MAX_CLIENT_NUM) 
			printf("read from server: %.*s\n", ret, buf);

			ret = nty_send(fd, buf, strlen(buf));
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

	listen(fd, 5);
	printf("listen port : %d\n", port);

	while (1) {
		socklen_t len = sizeof(struct sockaddr_in);
		int cli_fd = nty_accept(fd, (struct sockaddr*)&remote, &len);
		
		nty_coroutine *read_co;
		nty_coroutine_create(&read_co, server_reader, &cli_fd);

		int value = add_shmvalue();
		if (value % 1000 == 999) {
			printf("client sum: %d\n", value);
		}

	}
	
}

int mulcore_entry(int begin_port) {

	nty_coroutine *co = NULL;

	int i = 0;
	unsigned short base_port = begin_port;
	for (i = 0;i < 10;i ++) {
		unsigned short *port = calloc(1, sizeof(unsigned short));
		*port = base_port + i;
		nty_coroutine_create(&co, server, port); ////////no run
	}

	nty_schedule_run(); //run

}

int process_bind(void) {

	int num = sysconf(_SC_NPROCESSORS_CONF);

	pid_t self_id = syscall(__NR_gettid);
	printf("selfid --> %d\n", self_id);

	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(self_id % num, &mask);

	sched_setaffinity(0, sizeof(mask), &mask);

	mulcore_entry(9096 + (self_id % num) * 10);

}


int get_shmvalue(void) {
	return *global_shmaddr;
}

int add_shmvalue(void) {
	int value = 0;
	int ret = -1;

	do {
		value = *global_shmaddr;
		ret = cmpxchg(global_shmaddr, (unsigned long)value, (unsigned long)(value+1), 4);
	} while (ret != value);

	return *global_shmaddr;
}

int sub_shmvalue(void) {
	int value = 0;
	int ret = -1;

	do {
		value = *global_shmaddr;
		ret = cmpxchg(global_shmaddr, (unsigned long)value, (unsigned long)(value-1), 4);
	} while (ret != value);

	return *global_shmaddr;
}


int init_shmvalue(void) {
	
	global_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT|0600);
	if (global_shmid < 0) {
		perror("shmget failed\n");
		return -1;
	}
	global_shmaddr = (int*)shmat(global_shmid, NULL, 0);
	if (global_shmaddr == (int*)-1) {
		perror("shmat addr error");
		return -1;
	}
	*global_shmaddr = 0;

}



int main(int argc, char *argv[]) {

	int ret = init_shmvalue();
	if (ret == -1) {
		printf("init share memory failed\n");
		return -1;
	}
	
	fork();
	fork();
	fork();

	process_bind();

}


