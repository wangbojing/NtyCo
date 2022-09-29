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

#include <arpa/inet.h>



#define NTY_SERVER_IPADDR		"127.0.0.1"
#define NTY_SERVER_PORT			9096

int init_client(void) {

	int clientfd = nty_socket(AF_INET, SOCK_STREAM, 0);
	if (clientfd <= 0) {
		printf("socket failed\n");
		return -1;
	}

	struct sockaddr_in serveraddr = {0};
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(NTY_SERVER_PORT);
	serveraddr.sin_addr.s_addr = inet_addr(NTY_SERVER_IPADDR);

	int result = nty_connect(clientfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (result != 0) {
		printf("connect failed\n");
		return -2;
	}

	return clientfd;
	
}

void client(void *arg) {

	int clientfd = init_client();
	char *buffer = "ntyco_client\r\n";

	while (1) {

		int length = nty_send(clientfd, buffer, strlen(buffer), 0);
		printf("echo length : %d\n", length);

		sleep(1);
	}

}



int main(int argc, char *argv[]) {
	nty_coroutine *co = NULL;

	nty_coroutine_create(&co, client, NULL);
	
	nty_schedule_run(); //run

	return 0;
}







