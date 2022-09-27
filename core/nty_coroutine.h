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


#ifndef __NTY_COROUTINE_H__
#define __NTY_COROUTINE_H__


#define _GNU_SOURCE
#include <dlfcn.h>

#define _USE_UCONTEXT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <netinet/tcp.h>

#ifdef _USE_UCONTEXT
#include <ucontext.h>
#endif

#include <sys/epoll.h>
#include <sys/poll.h>

#include <errno.h>

#include "nty_queue.h"
#include "nty_tree.h"

#define NTY_CO_MAX_EVENTS		(1024*1024)
#define NTY_CO_MAX_STACKSIZE	(128*1024) // {http: 16*1024, tcp: 4*1024}

#define BIT(x)	 				(1 << (x))
#define CLEARBIT(x) 			~(1 << (x))

#define CANCEL_FD_WAIT_UINT64	1

typedef void (*proc_coroutine)(void *);


typedef enum {
	NTY_COROUTINE_STATUS_WAIT_READ,
	NTY_COROUTINE_STATUS_WAIT_WRITE,
	NTY_COROUTINE_STATUS_NEW,
	NTY_COROUTINE_STATUS_READY,
	NTY_COROUTINE_STATUS_EXITED,
	NTY_COROUTINE_STATUS_BUSY,
	NTY_COROUTINE_STATUS_SLEEPING,
	NTY_COROUTINE_STATUS_EXPIRED,
	NTY_COROUTINE_STATUS_FDEOF,
	NTY_COROUTINE_STATUS_DETACH,
	NTY_COROUTINE_STATUS_CANCELLED,
	NTY_COROUTINE_STATUS_PENDING_RUNCOMPUTE,
	NTY_COROUTINE_STATUS_RUNCOMPUTE,
	NTY_COROUTINE_STATUS_WAIT_IO_READ,
	NTY_COROUTINE_STATUS_WAIT_IO_WRITE,
	NTY_COROUTINE_STATUS_WAIT_MULTI
} nty_coroutine_status;

typedef enum {
	NTY_COROUTINE_COMPUTE_BUSY,
	NTY_COROUTINE_COMPUTE_FREE
} nty_coroutine_compute_status;

typedef enum {
	NTY_COROUTINE_EV_READ,
	NTY_COROUTINE_EV_WRITE
} nty_coroutine_event;


LIST_HEAD(_nty_coroutine_link, _nty_coroutine);
TAILQ_HEAD(_nty_coroutine_queue, _nty_coroutine);

RB_HEAD(_nty_coroutine_rbtree_sleep, _nty_coroutine);
RB_HEAD(_nty_coroutine_rbtree_wait, _nty_coroutine);



typedef struct _nty_coroutine_link nty_coroutine_link;
typedef struct _nty_coroutine_queue nty_coroutine_queue;

typedef struct _nty_coroutine_rbtree_sleep nty_coroutine_rbtree_sleep;
typedef struct _nty_coroutine_rbtree_wait nty_coroutine_rbtree_wait;


#ifndef _USE_UCONTEXT
typedef struct _nty_cpu_ctx {
	void *esp; //
	void *ebp;
	void *eip;
	void *edi;
	void *esi;
	void *ebx;
	void *r1;
	void *r2;
	void *r3;
	void *r4;
	void *r5;
} nty_cpu_ctx;
#endif

///
typedef struct _nty_schedule {
	uint64_t birth;
#ifdef _USE_UCONTEXT
	ucontext_t ctx;
#else
	nty_cpu_ctx ctx;
#endif
	void *stack;
	size_t stack_size;
	int spawned_coroutines;
	uint64_t default_timeout;
	struct _nty_coroutine *curr_thread;
	int page_size;

	int poller_fd;
	int eventfd;
	struct epoll_event eventlist[NTY_CO_MAX_EVENTS];
	int nevents;

	int num_new_events;
	pthread_mutex_t defer_mutex;

	nty_coroutine_queue ready;
	nty_coroutine_queue defer;

	nty_coroutine_link busy;
	
	nty_coroutine_rbtree_sleep sleeping;
	nty_coroutine_rbtree_wait waiting;

	//private 

} nty_schedule;

typedef struct _nty_coroutine {

	//private
	
#ifdef _USE_UCONTEXT
	ucontext_t ctx;
#else
	nty_cpu_ctx ctx;
#endif
	proc_coroutine func;
	void *arg;
	void *data;
	size_t stack_size;
	size_t last_stack_size;
	
	nty_coroutine_status status;
	nty_schedule *sched;

	uint64_t birth;
	uint64_t id;
#if CANCEL_FD_WAIT_UINT64
	int fd;
	unsigned short events;  //POLL_EVENT
#else
	int64_t fd_wait;
#endif
	char funcname[64];
	struct _nty_coroutine *co_join;

	void **co_exit_ptr;
	void *stack;
	void *ebp;
	uint32_t ops;
	uint64_t sleep_usecs;

	RB_ENTRY(_nty_coroutine) sleep_node;
	RB_ENTRY(_nty_coroutine) wait_node;

	LIST_ENTRY(_nty_coroutine) busy_next;

	TAILQ_ENTRY(_nty_coroutine) ready_next;
	TAILQ_ENTRY(_nty_coroutine) defer_next;
	TAILQ_ENTRY(_nty_coroutine) cond_next;

	TAILQ_ENTRY(_nty_coroutine) io_next;
	TAILQ_ENTRY(_nty_coroutine) compute_next;

	struct {
		void *buf;
		size_t nbytes;
		int fd;
		int ret;
		int err;
	} io;

	struct _nty_coroutine_compute_sched *compute_sched;
	int ready_fds;
	struct pollfd *pfds;
	nfds_t nfds;
} nty_coroutine;


typedef struct _nty_coroutine_compute_sched {
#ifdef _USE_UCONTEXT
	ucontext_t ctx;
#else
	nty_cpu_ctx ctx;
#endif
	nty_coroutine_queue coroutines;

	nty_coroutine *curr_coroutine;

	pthread_mutex_t run_mutex;
	pthread_cond_t run_cond;

	pthread_mutex_t co_mutex;
	LIST_ENTRY(_nty_coroutine_compute_sched) compute_next;
	
	nty_coroutine_compute_status compute_status;
} nty_coroutine_compute_sched;

extern pthread_key_t global_sched_key;
static inline nty_schedule *nty_coroutine_get_sched(void) {
	return pthread_getspecific(global_sched_key);
}

static inline uint64_t nty_coroutine_diff_usecs(uint64_t t1, uint64_t t2) {
	return t2-t1;
}

static inline uint64_t nty_coroutine_usec_now(void) {
	struct timeval t1 = {0, 0};
	gettimeofday(&t1, NULL);

	return t1.tv_sec * 1000000 + t1.tv_usec;
}



int nty_epoller_create(void);


void nty_schedule_cancel_event(nty_coroutine *co);
void nty_schedule_sched_event(nty_coroutine *co, int fd, nty_coroutine_event e, uint64_t timeout);

void nty_schedule_desched_sleepdown(nty_coroutine *co);
void nty_schedule_sched_sleepdown(nty_coroutine *co, uint64_t msecs);

nty_coroutine* nty_schedule_desched_wait(int fd);
void nty_schedule_sched_wait(nty_coroutine *co, int fd, unsigned short events, uint64_t timeout);

void nty_schedule_run(void);

int nty_epoller_ev_register_trigger(void);
int nty_epoller_wait(struct timespec t);
int nty_coroutine_resume(nty_coroutine *co);
void nty_coroutine_free(nty_coroutine *co);
int nty_coroutine_create(nty_coroutine **new_co, proc_coroutine func, void *arg);
void nty_coroutine_yield(nty_coroutine *co);

void nty_coroutine_sleep(uint64_t msecs);


int nty_socket(int domain, int type, int protocol);
int nty_accept(int fd, struct sockaddr *addr, socklen_t *len);
ssize_t nty_recv(int fd, void *buf, size_t len, int flags);
ssize_t nty_send(int fd, const void *buf, size_t len, int flags);
int nty_close(int fd);
int nty_poll(struct pollfd *fds, nfds_t nfds, int timeout);
int nty_connect(int fd, struct sockaddr *name, socklen_t namelen);

ssize_t nty_sendto(int fd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t nty_recvfrom(int fd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);


#define COROUTINE_HOOK 

#ifdef  COROUTINE_HOOK


typedef int (*socket_t)(int domain, int type, int protocol);
extern socket_t socket_f;

typedef int(*connect_t)(int, const struct sockaddr *, socklen_t);
extern connect_t connect_f;

typedef ssize_t(*read_t)(int, void *, size_t);
extern read_t read_f;


typedef ssize_t(*recv_t)(int sockfd, void *buf, size_t len, int flags);
extern recv_t recv_f;

typedef ssize_t(*recvfrom_t)(int sockfd, void *buf, size_t len, int flags,
        struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_t recvfrom_f;

typedef ssize_t(*write_t)(int, const void *, size_t);
extern write_t write_f;

typedef ssize_t(*send_t)(int sockfd, const void *buf, size_t len, int flags);
extern send_t send_f;

typedef ssize_t(*sendto_t)(int sockfd, const void *buf, size_t len, int flags,
        const struct sockaddr *dest_addr, socklen_t addrlen);
extern sendto_t sendto_f;

typedef int(*accept_t)(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
extern accept_t accept_f;

// new-syscall
typedef int(*close_t)(int);
extern close_t close_f;


int init_hook(void);


/*

typedef int(*fcntl_t)(int __fd, int __cmd, ...);
extern fcntl_t fcntl_f;

typedef int (*getsockopt_t)(int sockfd, int level, int optname,
        void *optval, socklen_t *optlen);
extern getsockopt_t getsockopt_f;

typedef int (*setsockopt_t)(int sockfd, int level, int optname,
        const void *optval, socklen_t optlen);
extern setsockopt_t setsockopt_f;

*/

#endif



#endif


