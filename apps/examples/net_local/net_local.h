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

#ifndef __EXAMPLE_NET_LOCAL_H
#define __EXAMPLE_NET_LOCAL_H

#define MAXLINE 80

const char *socket_path = "/dev/server.socket";

int net_local_server_main(int argc, char *argv[]);
int net_local_client_main(int argc, char *argv[]);
int client_talk(int sockfd, const char *func, const char *str);
void server_talk(int sockfd, const char *func);

#endif
