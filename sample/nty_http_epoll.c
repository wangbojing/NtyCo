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

#include <errno.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/poll.h>

#define __USE_GNU

#include <sched.h>
#include <string.h>
#include <pthread.h>

#include <ctype.h>

#define MAX_CLIENT_NUM			1000000
#define TOTALFDS			16

typedef struct _shm_area {
	int totalfds[TOTALFDS];
	char cpu_lb[TOTALFDS];
	//mtx : default 0
	// 1 : lock -- del epoll
	// 2 : lock -- del complete
	// 3 : unlock -- add
	// 0 : unlock -- add complete
	int accept_mtx;
} shm_area;

static shm_area *global_shmaddr = NULL;
static int global_shmid = -1;

int cpu_size = 0;
int accept_disable = 1000;
int enable_accept = 1;
pid_t self_id = 0;



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

int atomic_add(volatile int *value, int add)
{
    __asm__ volatile (

         "lock;"
    "    addl  %0, %1;   "

    : "+r" (add) : "m" (*value) : "cc", "memory");

    return add;
}

int atomic_sub(volatile int *value, int sub)
{
    __asm__ volatile (

         "lock;"
    "    subl  %0, %1;   "

    : "+r" (sub) : "m" (*value) : "cc", "memory");

    return sub;
}





#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: ntyco_httpd/0.1.0\r\n"
#define STDIN   0
#define STDOUT  1
#define STDERR  2

#define ENABLE_NTYCO	0

#if ENABLE_NTYCO

#define socket 	nty_socket 
#define accept 	nty_accept 
#define recv(a, b, c, d) 	nty_recv(a, b, c, d)
#define send(a, b, c, d) 	nty_send(a, b, c, d) 

#define MAX_BUFFER_LENGTH	1024



#endif

#define INC_COUNTFD  do {				\
	atomic_add(&global_shmaddr->totalfds[self_id % cpu_size], 1);	\
} while (0)


#define DEC_COUNTFD  do {				\
	atomic_sub(&global_shmaddr->totalfds[self_id % cpu_size], 1);	\
} while (0)

int get_countfd(void) {	
	return global_shmaddr->totalfds[self_id % cpu_size];	
}

int max_countfd(void) {

	int count = -1;
	int i = 0;

	for (i = 0;i < cpu_size;i ++) {
		if (count < global_shmaddr->totalfds[i]) {
			count = global_shmaddr->totalfds[i];
		}
	}
	return count;
}

int min_countfd(void) {

	int count = 0xffffffff;
	int i = 0;

	for (i = 0;i < cpu_size;i ++) {
		if (count > global_shmaddr->totalfds[i]) {
			count = global_shmaddr->totalfds[i];
		}
	}
	return count;
}

int compare_out_countfd(void) {

	int current = get_countfd();
	int min = min_countfd();

	if ((current * 7 / 8) > min) {
		return 1;
	} else {
		return 0;
	}
}

int compare_in_countfd(void) {

	int current = get_countfd();
	int max = max_countfd();

	if ((current * 8 / 7) < max) {
		return 1;
	} else {
		return 0;
	}
}

void print_countfds(void) {

	int i = 0;
	for (i = 0;i < cpu_size;i ++) {
		printf("%5d : %5d ", i, global_shmaddr->totalfds[i]);
	}
	printf("\n");
}

void lock_accept(void) {
	
	global_shmaddr->cpu_lb[self_id % cpu_size] = 1;

	int count = 0xffffffff;
	int i = 0;

	for (i = 0;i < cpu_size;i ++) {
		if (count > global_shmaddr->totalfds[i]) {
			count = global_shmaddr->totalfds[i];
		}
	}

	for (i = 0;i < cpu_size;i ++) {
		if (count == global_shmaddr->totalfds[i]) {
			global_shmaddr->cpu_lb[i] = 3;
		}
	}
	
}

char read_accept(void) {
	return global_shmaddr->cpu_lb[self_id % cpu_size];
}

void write_accept(char state) { //0, 1, 2, 3		
	global_shmaddr->cpu_lb[self_id % cpu_size] = state;
}

int lock(void) {

	return cmpxchg(&global_shmaddr->accept_mtx, 0, 1, 4); //zero:success, non-zero:failed

}

void unlock(void) {

	global_shmaddr->accept_mtx = 0;

}


void accept_request(int fd, int epfd);
void bad_request(int);
void cat(int);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int);
void not_found(int);
void serve_file(int);
int startup(u_short *);
void unimplemented(int);


ssize_t nsend(int fd, const void *buf, size_t len, int flags) {
	
	int sent = 0;

	int ret = send(fd, ((char*)buf)+sent, len-sent, flags);
	if (ret == 0) return ret;
	if (ret > 0) sent += ret;

	while (sent < len) {
#if 0
        struct pollfd fds;
		fds.fd = fd;
		fds.events = POLLIN | POLLERR | POLLHUP;
        poll(&fds, 1, -1);
#endif
		ret = send(fd, ((char*)buf)+sent, len-sent, flags);
		//printf("send --> len : %d\n", ret);
		if (ret <= 0) {	
			if (errno == EAGAIN) continue;
			
			break;
		}
		sent += ret;
	}

	if (ret <= 0 && sent == 0) return ret;
	
	return sent;
}

static int ntySetNonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}

static int ntySetReUseAddr(int fd) {
	int reuse = 1;
	return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
}


/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(int fd, int epfd)
{
    int client = fd;
	
    char buf[1024];
    size_t numchars;
    char method[16];
    char url[32];
    char path[64];
    size_t i, j;
    struct stat st;
    int cgi = 0;      /* becomes true if server decides this is a CGI
                       * program */
    char *query_string = NULL;

    numchars = recv(client, buf, sizeof(buf), 0);
    if (numchars == -1) {
        //printf("recv errno : %d\n", errno);
		if (errno == ECONNRESET) {
			close(client);

			struct epoll_event ev;
			ev.events = EPOLLIN  | EPOLLET;
			ev.data.fd = client;
			epoll_ctl(epfd, EPOLL_CTL_DEL, client, &ev);

			DEC_COUNTFD;
		}
    } else if (numchars == 0) {
        close(client);

		struct epoll_event ev;
		ev.events = EPOLLIN  | EPOLLET;
		ev.data.fd = client;
		epoll_ctl(epfd, EPOLL_CTL_DEL, client, &ev);

		DEC_COUNTFD;
    } else {
        //printf("recv numchars : %ld\n", numchars);
    }
    //serve_file(client, path);

}

#if 0
#define HTML_PAGE 		"<HTML> \
						<TITLE>Index</TITLE> \
						<BODY> \
						<P>Welcome to J. David's webserver.\
						<H1>CGI demo \
						<FORM ACTION=\"color.cgi\" METHOD=\"POST\">\
						Enter a color: <INPUT TYPE=\"text\" NAME=\"color\">\
						<INPUT TYPE=\"submit\">\
						</FORM>\
						</BODY>\
						</HTML>"

#else
#define HTML_PAGE       "<!DOCTYPE html> \
<html> \
<head> \
<title>Welcome to nginx!</title> \
<style> \
    body { \
        width: 35em;\
        margin: 0 auto;\
        font-family: Tahoma, Verdana, Arial, sans-serif;\
    }\
</style>\
</head>\
<body>\
<h1>Welcome to nginx!</h1>\
<p>If you see this page, the nginx web server is successfully installed and\
working. Further configuration is required.</p>\
\
<p>For online documentation and support please refer to\
<a href=\"http://nginx.org/\">nginx.org</a>.<br/>\
Commercial support is available at\
<a href=\"http://nginx.com/\">nginx.com</a>.</p>\
\
<p><em>Thank you for using nginx.</em></p>\
</body>\
</html>"
#endif

void cat(int client)
{

    //char buf[1024] =  HTML_PAGE;

	int ret = send(client, HTML_PAGE, strlen(HTML_PAGE), 0);
    if (ret == -1) {
        printf("send : errno : %d\n", errno);
    }  else {
        printf("cat send ret : %d\n", ret);
    }
	
}

void error_die(const char *sc)
{
    printf("%s\n", sc);
    exit(1);
}

void headers(int client)
{
    char buf[1024] = {0};
	char content[128] = {0};

	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	strcat(buf, SERVER_STRING);
	strcat(buf, "Content-Type: text/html\r\n");
#if 0
	strcat(buf, "Transfer-Encoding: chunked\r\n");
#else 
	sprintf(content, "Content-Length: %ld\r\n", strlen(HTML_PAGE));
	strcat(buf, content);
#endif 
	strcat(buf, "\r\n");

    strcat(buf, HTML_PAGE);
	
	int ret = send(client, buf, strlen(buf), 0);
    if (ret == -1) {
        printf("send : errno : %d\n", errno);
    } else {
        //printf("headers send ret : %d\n", ret);
    }
    

}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client)
{
    headers(client);
    //cat(client);
   
}

#define MAX_EPOLLSIZE		10240

void server(void *arg) {

	int fd = *(int *)arg;

	int epfd = epoll_create(1);
    struct epoll_event events[MAX_EPOLLSIZE];

    struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
	int acc = 1; //

	while (1) {
#if 0
		char lock = read_accept();
		if (lock == 1) { //lock
			ev.events = EPOLLIN;
			ev.data.fd = fd;
			epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);

			write_accept(2); //add complete
			
		} else if (acc == 0 && lock == 3) {
			ev.events = EPOLLIN;
			ev.data.fd = fd;
			epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
			
			write_accept(0);
		}
#endif
        int nready = epoll_wait(epfd, events, MAX_EPOLLSIZE, 1);
		if (nready == -1) {
			if (errno == EINTR) {
				printf("errno : EINTR\n");
				continue;
			}
			continue;
		} 

		int i = 0;
		for (i = 0;i < nready;i ++) {
			int sockfd = events[i].data.fd;
			if (sockfd == fd) {

				struct sockaddr_in client_addr;
				memset(&client_addr, 0, sizeof(struct sockaddr_in));
				socklen_t client_len = sizeof(client_addr);
#if 0
				if (get_countfd() % accept_disable == 999) {
					lock_accept();
					acc = 0;
				}
				char lock = read_accept();
				if (lock != 0 && lock != 3) continue;
#endif
				if (lock()) {
					continue;
				}
				int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
				unlock();

				if (clientfd < 0) {
					if (errno == EAGAIN) {
						//printf("accept : EAGAIN\n");
						continue;
					} else if (errno == ECONNABORTED) {
						printf("accept : ECONNABORTED\n");
						
					} else if (errno == EMFILE || errno == ENFILE) {
						printf("accept : EMFILE || ENFILE\n");
					}
					return ;
				}

				INC_COUNTFD;

				ntySetNonblock(clientfd);
                ntySetReUseAddr(clientfd);

				struct epoll_event ev;
				ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
				ev.data.fd = clientfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);

			}
			else if (events[i].events & EPOLLIN) {

				accept_request(sockfd, epfd);
#if 1
				//struct epoll_event ev;
				ev.events = EPOLLOUT  | EPOLLET | EPOLLONESHOT;
				ev.data.fd = sockfd;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
				
				//close(sockfd);
#endif
			}       
            else if (events[i].events & EPOLLOUT) {

				serve_file(sockfd);

				//close(sockfd);

				//struct epoll_event ev;
				ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
				ev.data.fd = sockfd;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);

				DEC_COUNTFD;
				
			}
			else if (events[i].events & (EPOLLHUP|EPOLLRDHUP|EPOLLERR)) {

				close(sockfd);

				printf(" EPOLLHUP | EPOLLRDHUP\n");
				//struct epoll_event ev;
				ev.events = EPOLLIN  | EPOLLET | EPOLLONESHOT;
				ev.data.fd = sockfd;
				epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, &ev);
				
				DEC_COUNTFD;
			}
		}

//		print_countfds();
	}

}

int mulcore_entry(int begin_port) {

	int i = 0;
	unsigned short base_port = begin_port;

	unsigned short *port = calloc(1, sizeof(unsigned short));
	*port = base_port;
	server(port);

}

int process_bind(int fd) {

	int num = sysconf(_SC_NPROCESSORS_CONF);

	self_id = syscall(__NR_gettid);
	//printf("selfid --> %d\n", self_id);

	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(self_id % num, &mask);

	sched_setaffinity(0, sizeof(mask), &mask);
    server(&fd);
	//mulcore_entry(9000);

}

int init_shmvalue(int num) {
	
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
	memset(global_shmaddr->totalfds, 0, TOTALFDS * sizeof(int));
	memset(global_shmaddr->cpu_lb, 0, TOTALFDS * sizeof(char));
	global_shmaddr->accept_mtx = 0;
}




int main(int argc, char *argv[]) {

    short port = 9000;
	struct sockaddr_in client_name;
	socklen_t  client_name_len = sizeof(client_name);

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;

    ntySetNonblock(fd);
    ntySetReUseAddr(fd);


	struct sockaddr_in local, remote;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = INADDR_ANY;
	bind(fd, (struct sockaddr*)&local, sizeof(struct sockaddr_in));

	listen(fd, 20);

	int num = sysconf(_SC_NPROCESSORS_CONF);
	cpu_size = num;
	init_shmvalue(num);
	
	int i = 0;
	pid_t pid=0;
	for(i = 0;i < num;i ++) {
		pid=fork();
		if(pid <= (pid_t) 0)
		{
		   usleep(1);
		   break;
		}
  	}
#if 1
	if (pid > 0) {
		printf("ntyco_httpd server running ...\n");
		getchar();
	} else if (pid == 0) {
		process_bind(fd);
	}
#else


	INC_COUNTFD;

	INC_COUNTFD;
	print_countfds();


#endif
}

