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
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>

#include <sys/stat.h>
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
	int accept_lock;
	struct timeval tv_begin;
} shm_area;

static shm_area *global_shmaddr = NULL;


#define MAX_CLIENT_NUM			1000000


struct timeval* get_shm_timevalue(void);
void set_shm_timevalue(struct timeval *tv);


int add_shmvalue(void);
int sub_shmvalue(void);

int init_shmvalue(void);

int lock_accept_mutex(void);
int unlock_accept_mutex(void);




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


#define ISspace(x) isspace((int)(x))

#define SERVER_STRING "Server: ntyco_httpd/0.1.0\r\n"
#define STDIN   0
#define STDOUT  1
#define STDERR  2

#define ENABLE_NTYCO	1

#if ENABLE_NTYCO

#define socket 	nty_socket 
#define accept 	nty_accept 
#define recv(a, b, c, d) 	nty_recv(a, b, c, d)
#define send(a, b, c, d) 	nty_send(a, b, c, d) 
#define close(a)	nty_close(a)

#define MAX_BUFFER_LENGTH	1024

#endif

void accept_request(void *);
void bad_request(int);
void cat(int, FILE *);
void cannot_execute(int);
void error_die(const char *);
void execute_cgi(int, const char *, const char *, const char *);
int get_line(int, char *, int);
void headers(int, const char *);
void not_found(int);
void serve_file(int, const char *);
int startup(u_short *);
void unimplemented(int);

int readline(char* allbuf, int level, char* linebuf) {    
	int len = strlen(allbuf);    

	for (;level < len; ++level)    {        
		if(allbuf[level]=='\r' && allbuf[level+1]=='\n')            
			return level+2;        
		else            
			*(linebuf++) = allbuf[level];    
	}    

	return -1;
}


/**********************************************************************/
/* A request has caused a call to accept() on the server port to
 * return.  Process the request appropriately.
 * Parameters: the socket connected to the client */
/**********************************************************************/
void accept_request(void *arg)
{
    int *pclient = (int*)arg;
	int client = *pclient;
	
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

    i = 0; j = 0;
    while (!ISspace(buf[i]) && (i < sizeof(method) - 1))
    {
        method[i] = buf[i];
        i++;
    }
    j=i;
    method[i] = '\0';

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return;
    }

    if (strcasecmp(method, "POST") == 0)
        cgi = 1;

    i = 0;
    while (ISspace(buf[j]) && (j < numchars))
        j++;
    while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < numchars))
    {
        url[i] = buf[j];
        i++; j++;
    }
    url[i] = '\0';

    if (strcasecmp(method, "GET") == 0)
    {
        query_string = url;
        while ((*query_string != '?') && (*query_string != '\0'))
            query_string++;
        if (*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }

    sprintf(path, "htdocs%s", url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");
    if (stat(path, &st) == -1) {
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
        not_found(client);
    }
    else
    {
        if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html");
        if ((st.st_mode & S_IXUSR) ||
                (st.st_mode & S_IXGRP) ||
                (st.st_mode & S_IXOTH)    )
            cgi = 1;

		cgi = 0;

        if (!cgi)
            serve_file(client, path);
        else
            execute_cgi(client, path, method, query_string);
    }

    close(client);
	free(pclient);
}


/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/

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

void cat(int client, FILE *resource)
{
#if ENABLE_NTYCO
    char buf[1024] =  HTML_PAGE;

	send(client, buf, strlen(buf), 0);

#else
	char buf[1024] = {0};
    fgets(buf, sizeof(buf), resource);
    while (!feof(resource))
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
#endif
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
    perror(sc);
    exit(1);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(int client, const char *path,
        const char *method, const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars = 1;
    int content_length = -1;

    buf[0] = 'A'; buf[1] = '\0';
    if (strcasecmp(method, "GET") == 0)
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));
    else if (strcasecmp(method, "POST") == 0) /*POST*/
    {
        numchars = get_line(client, buf, sizeof(buf));
        while ((numchars > 0) && strcmp("\n", buf))
        {
            buf[15] = '\0';
            if (strcasecmp(buf, "Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = get_line(client, buf, sizeof(buf));
        }
        if (content_length == -1) {
            bad_request(client);
            return;
        }
    }
    else/*HEAD or other*/
    {
    }


    if (pipe(cgi_output) < 0) {
        cannot_execute(client);
        return;
    }
    if (pipe(cgi_input) < 0) {
        cannot_execute(client);
        return;
    }

    if ( (pid = fork()) < 0 ) {
        cannot_execute(client);
        return;
    }
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    if (pid == 0)  /* child: CGI script */
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1], STDOUT);
        dup2(cgi_input[0], STDIN);
        close(cgi_output[0]);
        close(cgi_input[1]);
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else {   /* POST */
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        execl(path, "", NULL);
        exit(0);
    } else {    /* parent */
        close(cgi_output[1]);
        close(cgi_input[0]);
        if (strcasecmp(method, "POST") == 0)
            for (i = 0; i < content_length; i++) {

				recv(client, &c, 1, 0);

                int len = write(cgi_input[1], &c, 1);
            }
        while (read(cgi_output[0], &c, 1) > 0)

			send(client, &c, 1, 0);


        close(cgi_output[0]);
        close(cgi_input[1]);
        waitpid(pid, &status, 0);
    }
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

    while ((i < size - 1) && (c != '\n'))
    {

		n = recv(sock, &c, 1, 0);

        if (n > 0)
        {
            if (c == '\r')
            {

                n = recv(sock, &c, 1, MSG_PEEK);

                if ((n > 0) && (c == '\n'))

					recv(sock, &c, 1, 0);

                else
                    c = '\n';
            }
            buf[i] = c;
            i++;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';

    return(i);
}

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(int client, const char *filename)
{
    char buf[1024] = {0};
    (void)filename;  /* could use filename to determine file type */
	
#if ENABLE_NTYCO
   

	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	strcat(buf, SERVER_STRING);
	strcat(buf, "Content-Type: text/html\r\n");
	strcat(buf, "\r\n");

	send(client, buf, strlen(buf), 0);
	
#else
	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
	
#endif
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(int client)
{
    char buf[1024];
#if ENABLE_NTYCO

	sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	strcat(buf, SERVER_STRING);
	strcat(buf, "Content-Type: text/html\r\n");
	strcat(buf, "\r\n");
	strcat(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	strcat(buf, "<BODY><P>The server could not fulfill\r\n");
	strcat(buf, "your request because the resource specified\r\n");
	strcat(buf, "is unavailable or nonexistent.\r\n");
	strcat(buf, "</BODY></HTML>\r\n");

	send(client, buf, strlen(buf), 0);

#else
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
#endif
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(int client, const char *filename)
{
    FILE *resource = NULL;
    int numchars = 1;
    char buf[1024];
	
#if (ENABLE_NTYCO == 0)
    buf[0] = 'A'; buf[1] = '\0';
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
        numchars = get_line(client, buf, sizeof(buf));
	resource = fopen(filename, "r");
    if (resource == NULL)
        not_found(client);
    else
    {
        headers(client, filename);
        cat(client, resource);
    }
    fclose(resource);
#else
	headers(client, filename);
    cat(client, resource);
#endif

    
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
int startup(u_short *port)
{
    int httpd = 0;
    int on = 1;
    struct sockaddr_in name;

    httpd = socket(PF_INET, SOCK_STREAM, 0);
    if (httpd == -1)
        error_die("socket");
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if ((setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)  
    {  
        error_die("setsockopt failed");
    }
    if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)
        error_die("bind");
    if (*port == 0)  /* if dynamically allocating a port */
    {
        socklen_t namelen = sizeof(name);
        if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
            error_die("getsockname");
        *port = ntohs(name.sin_port);
    }
    if (listen(httpd, 5) < 0)
        error_die("listen");

	
    return(httpd);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(int client)
{
    char buf[1024];
#if ENABLE_NTYCO
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	strcat(buf, SERVER_STRING);
	strcat(buf, "Content-Type: text/html\r\n");
	strcat(buf, "\r\n");
	strcat(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	strcat(buf, "</TITLE></HEAD>\r\n");
	strcat(buf, "<BODY><P>HTTP request method not supported.\r\n");
	strcat(buf, "</BODY></HTML>\r\n");

	send(client, buf, strlen(buf), 0);
#else
	sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
#endif
}

void server(void *arg) {

	unsigned short port = *(unsigned short *)arg;
	struct sockaddr_in client_name;
	socklen_t  client_name_len = sizeof(client_name);
	free(arg);

	int fd = nty_socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return ;

	struct sockaddr_in local, remote;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = INADDR_ANY;
	bind(fd, (struct sockaddr*)&local, sizeof(struct sockaddr_in));

	listen(fd, 20);

	while (1) {
		int *client_sock = (int *)malloc(sizeof(int));
        *client_sock = accept(fd,
                (struct sockaddr *)&client_name,
                &client_name_len);
        if (*client_sock == -1)
            error_die("accept");
        /* accept_request(&client_sock); */
#if 0
		printf(" %d.%d.%d.%d:%d clientfd:%d --> New Client Connected \n", 
			*(unsigned char*)(&client_name.sin_addr.s_addr), *((unsigned char*)(&client_name.sin_addr.s_addr)+1),													
			*((unsigned char*)(&client_name.sin_addr.s_addr)+2), *((unsigned char*)(&client_name.sin_addr.s_addr)+3),													
			client_name.sin_port, *client_sock);
#endif
		nty_coroutine *read_co;
		nty_coroutine_create(&read_co, accept_request, client_sock);
	}

}

int mulcore_entry(int begin_port) {

	nty_coroutine *co = NULL;

	int i = 0;
	unsigned short base_port = begin_port;
	
	unsigned short *port = calloc(1, sizeof(unsigned short));
	*port = base_port + i;
	nty_coroutine_create(&co, server, port); ////////no run
	

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

int lock_accept_mutex(void) {

	int ret = cmpxchg(&global_shmaddr->accept_lock, (unsigned long)0, (unsigned long)1, 4);
	
	return ret; //success 0, failed 1.
}

int unlock_accept_mutex(void) {

	int ret = cmpxchg(&global_shmaddr->accept_lock, (unsigned long)1, (unsigned long)0, 4);
	
	return ret; //unlock 1, lock 0
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

	int num = sysconf(_SC_NPROCESSORS_CONF);
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

	if (pid > 0) {
		printf("ntyco_httpd server running ...\n");
		getchar();
	} else if (pid == 0) {
		process_bind();
	}

}

