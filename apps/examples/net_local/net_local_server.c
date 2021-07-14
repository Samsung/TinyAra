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
#include <ctype.h>

#include "net_local.h"

int server_talk(int sockfd, const char *func)
{
	int i, n;
	char buf[MAXLINE + 1];
	while (1) {
		n = read(sockfd, buf, MAXLINE);
		if (n < 0) {
			perror("read");
			return -1;
		} else if (n == 0) {
			break;
		}

		buf[n] = '\0';
		printf("[%s] receive: %s\n", func, buf);
		for (i = 0; i < n; i++) {
			buf[i] = toupper(buf[i]);
		}
		printf("[%s] respond: %s\n", func, buf);
		write(sockfd, buf, n);
	}

	return 0;
}

int net_local_server_main(int argc, char *argv[])
{
	struct sockaddr_un server_sun;
	int listenfd;
	int connfd;
	int size;

	printf("[%s] enter\n", __FUNCTION__);

	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket\n");
		return -1;
	}

	memset(&server_sun, 0, sizeof(server_sun));
	server_sun.sun_family = AF_UNIX;
	strcpy(server_sun.sun_path, socket_path);
	size = offsetof(struct sockaddr_un, sun_path) + strlen(server_sun.sun_path);
	unlink(socket_path);

	printf("[%s] Binding ...\n", __FUNCTION__);

	if (bind(listenfd, (struct sockaddr *)&server_sun, size) < 0) {
		perror("bind");
		goto errout;
	}

	printf("[%s] Listening ...\n", __FUNCTION__);

	if (listen(listenfd, 20) < 0) {
		perror("listen");
		goto errout;
	}

	printf("[%s] Accepting ...\n", __FUNCTION__);

	if ((connfd = accept(listenfd, NULL, NULL)) < 0) {
		perror("accept");
		goto errout;
	}

	printf("[%s] Accepted new connection, fd %d\n", __FUNCTION__, connfd);

	server_talk(connfd, __FUNCTION__);

	close(connfd);

errout:
	close(listenfd);

	printf("[%s] exit\n", __FUNCTION__);
	return 0;
}
