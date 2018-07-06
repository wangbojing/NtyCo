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

#include "nty_coroutine.h"



static uint32_t nty_pollevent_2epoll( short events )
{
	uint32_t e = 0;	
	if( events & POLLIN ) 	e |= EPOLLIN;
	if( events & POLLOUT )  e |= EPOLLOUT;
	if( events & POLLHUP ) 	e |= EPOLLHUP;
	if( events & POLLERR )	e |= EPOLLERR;
	if( events & POLLRDNORM ) e |= EPOLLRDNORM;
	if( events & POLLWRNORM ) e |= EPOLLWRNORM;
	return e;
}
static short nty_epollevent_2poll( uint32_t events )
{
	short e = 0;	
	if( events & EPOLLIN ) 	e |= POLLIN;
	if( events & EPOLLOUT ) e |= POLLOUT;
	if( events & EPOLLHUP ) e |= POLLHUP;
	if( events & EPOLLERR ) e |= POLLERR;
	if( events & EPOLLRDNORM ) e |= POLLRDNORM;
	if( events & EPOLLWRNORM ) e |= POLLWRNORM;
	return e;
}
/*
 * nty_poll_inner --> 1. sockfd--> epoll, 2 yield, 3. epoll x sockfd
 * fds : 
 */

static int nty_poll_inner(struct pollfd *fds, nfds_t nfds, int timeout) {

	if (timeout == 0)
	{
		return poll(fds, nfds, timeout);
	}
	if (timeout < 0)
	{
		timeout = INT_MAX;
	}

	nty_schedule *sched = nty_coroutine_get_sched();
	nty_coroutine *co = sched->curr_thread;
	
	int i = 0;
	for (i = 0;i < nfds;i ++) {
	
		struct epoll_event ev;
		ev.events = nty_pollevent_2epoll(fds[i].events);
		ev.data.fd = fds[i].fd;
		epoll_ctl(sched->poller_fd, EPOLL_CTL_ADD, fds[i].fd, &ev);

		co->events = fds[i].events;
		nty_schedule_sched_wait(co, fds[i].fd, fds[i].events, timeout);
	}
	nty_coroutine_yield(co); 

	for (i = 0;i < nfds;i ++) {
	
		struct epoll_event ev;
		ev.events = nty_pollevent_2epoll(fds[i].events);
		ev.data.fd = fds[i].fd;
		epoll_ctl(sched->poller_fd, EPOLL_CTL_DEL, fds[i].fd, &ev);

		nty_schedule_desched_wait(fds[i].fd);
	}

	return nfds;
}


int nty_socket(int domain, int type, int protocol) {

	int fd = socket(domain, type, protocol);
	if (fd == -1) {
		printf("Failed to create a new socket\n");
		return -1;
	}
	int ret = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (ret == -1) {
		close(ret);
		return -1;
	}
	return fd;
}

int nty_accept(int fd, struct sockaddr *addr, socklen_t *len) {
	int sockfd = -1;
	int timeout = 1;
	nty_coroutine *co = nty_coroutine_get_sched()->curr_thread;
	
	while (1) {
		struct pollfd fds;
		fds.fd = fd;
		fds.events = POLLIN;
		nty_poll_inner(&fds, 1, timeout);

		sockfd = accept(fd, addr, len);
		if (sockfd <= 0) {
			if (errno == ENFILE || 
				errno == EWOULDBLOCK ||
				errno == EMFILE) {
				timeout *= 2;
				if (timeout >= 1024) timeout = 1024;
				continue;
			} else if (errno == ECONNABORTED) {
				printf("Cannot accept connection\n");
				continue;
			} else {
				printf("Cannot accept connection\n");
				return -1;
			}
		} else {
			break;
		}
	}

	int ret = fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if (ret == -1) {
		close(sockfd);
		return -1;
	}
	return sockfd;
}


int nty_recv(int fd, void *buf, int length) {
	
	struct pollfd fds;
	fds.fd = fd;
	fds.events = POLLIN;

	nty_poll_inner(&fds, 1, 1);

	int ret = recv(fd, buf, length, 0);
	if (ret < 0) {
		printf("recv error : %d, ret : %d\n", errno, ret);
		if (errno == EAGAIN) return ret;
		assert(0);
	}
	return ret;
}


int nty_send(int fd, const void *buf, int length) {
	
	int sent = 0;

	while (sent < length) {
		struct pollfd fds;
		fds.fd = fd;
		fds.events = POLLOUT;

		nty_poll_inner(&fds, 1, 1);
		int ret = send(fd, ((char*)buf)+sent, length-sent, 0);
		if (ret <= 0) {
			if (errno == EAGAIN) continue;
			else if (errno == ECONNRESET) {
				return ret;
			}
			printf("send errno : %d, ret : %d\n", errno, ret);
			assert(0);
		}
		sent += ret;
	}
}


int nty_close(int fd) {

	nty_schedule *sched = nty_coroutine_get_sched();

	nty_coroutine *co = sched->curr_thread;
	if (co) {
		TAILQ_INSERT_TAIL(&nty_coroutine_get_sched()->ready, co, ready_next);
		co->status |= BIT(NTY_COROUTINE_STATUS_FDEOF);
	}
	
	return close(fd);
}

