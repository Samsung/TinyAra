/****************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "net_local.h"

int client_talk(int sockfd, const char *func, const char *str)
{
	int n;
	char buf[MAXLINE + 1];
	printf("[%s] send: %s\n", func, str);
	n = write(sockfd, (const void *)str, strlen(str));
	if (n < 0) {
		perror("write");
		return -1;
	}

	n = read(sockfd, buf, MAXLINE);
	if (n < 0) {
		perror("read");
		return -1;
	} else {
		buf[n] = '\0';
		printf("[%s] recv: %s\n", func, buf);
		return strncasecmp(buf, str, n);
	}
}

int net_local_client_main(int argc, char *argv[])
{
	struct sockaddr_un server_sun;
	int sockfd;
	int len;

	printf("[%s] enter\n", __FUNCTION__);

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		printf("client socket error");
		return -1;
	}

	memset(&server_sun, 0, sizeof(server_sun));
	server_sun.sun_family = AF_UNIX;
	strcpy(server_sun.sun_path, server_path);
	len = offsetof(struct sockaddr_un, sun_path) + strlen(server_sun.sun_path);

	printf("[%s] Connecting ...\n", __FUNCTION__);
	if (connect(sockfd, (struct sockaddr *)&server_sun, len) < 0) {
		perror("connect");
		goto errout;
	}

	client_talk(sockfd, __FUNCTION__, "Hello, Unix Domain Socket!");

	shutdown(sockfd, SHUT_RDWR);
errout:
	close(sockfd);
	printf("[%s] exit\n", __FUNCTION__);
	return 0;
}
