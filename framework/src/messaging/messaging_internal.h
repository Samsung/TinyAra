/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
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
#include <tinyara/compiler.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <messaging/messaging.h>

#define MSG_ALLOC(a) malloc(a)
#define MSG_FREE(a) free(a)
#ifdef CONFIG_CPP_HAVE_VARARGS
#define MSG_ASPRINTF(p, f, ...) asprintf(p, f, ##__VA_ARGS__)
#else
#define MSG_ASPRINTF asprintf
#endif

#define MSG_HEADER_SIZE (sizeof(pid_t) + sizeof(int))

#define MAX_PORT_NAME_SIZE 64
/* @cond
 * @internal
 */
/**
 * @brief The type of handling message
 * @details MSG_HANDLE_SAVE    : For saving receiver information\n
 * MSG_HANDLE_READ : For reading receiver information\n
 * MSG_HANDLE_REMOVE : For removing receiver information
 */
enum msg_handle_type_e {
	MSG_HANDLE_SAVE = 0,
	MSG_HANDLE_READ = 1,
	MSG_HANDLE_REMOVE = 2,
	MSG_HANDLE_TYPE_MAX
};
typedef enum msg_handle_type_e msg_handle_type_t;

/**
 * @brief The type of sending message
 * @details MSG_SEND_SYNC : Unicast send message type with sync mode\n
 * MSG_SEND_ASYNC : Unicast send message type with async mode
 * MSG_SEND_MULTI : Multicast send message type
 */
enum msg_send_type_e {
	MSG_SEND_NOREPLY = 0,
	MSG_SEND_SYNC = 1,
	MSG_SEND_ASYNC = 2,
	MSG_SEND_MULTI = 3,
	MSG_SEND_TYPE_MAX
};
typedef enum msg_send_type_e msg_send_type_t;

/**
 * @brief The internal structure for callback information and mq descriptor
 */
struct msg_recv_info_s {
	mqd_t mqdes;
	msg_callback_t user_cb;
	char *cb_data;
};
typedef struct msg_recv_info_s msg_recv_info_t;

/**
 * @brief The internal structure for saving message port information. It will be used when cleanup.
 */
struct msg_port_info_s {
	struct msg_port_info_s *flink;
	char name[MAX_PORT_NAME_SIZE];
	mqd_t mqdes;
	void *data;
	pid_t pid;
};
typedef struct msg_port_info_s msg_port_info_t;

/**
 * @brief Internal function for setting callback function to the messaging signal.
 */
int messaging_set_notify_signal(int signo, _sa_sigaction_t cb_func);
/**
 * @brief Internal function for setting mq notification.
 */
int messaging_set_notification(int signo, msg_recv_info_t *data);
/**
 * @brief Internal function for save/read the message information when sending and receiving the message.
 */
int messaging_handle_data(msg_handle_type_t type, const char *port_name, int *data_arr, int *recv_cnt);
/**
 * @brief Internal function for unicast and multicast send APIs.
 */
int messaging_send_internal(const char *port_name, int msg_type, msg_send_data_t *send_data, msg_callback_info_t *cb_info);
/**
 * @brief Internal function for receiving APIs.
 */
int messaging_recv_internal(const char *port_name, msg_recv_buf_t *recv_buf, msg_callback_info_t *cb_info);

/*
 *@endcond
 */
