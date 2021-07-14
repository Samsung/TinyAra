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

#ifndef __AUDIO_COMMON_H__
#define __AUDIO_COMMON_H__

#include <media/MediaTypes.h>

#define DEFAULT_STACK_SIZE 4096
#define HTTP_STREAM 0
#define HTTP_HEADER 1

#define CIRCULAR_BUFFER_SIZE 1024 * 1024 // 1 Mb
#define BUFFERING_TIME_VALUE 5			 // 2 msec

#define USED_CURL_TRACE 0

using namespace media;

struct msg_recv_buf_s {
	char *buf;
	int buflen;
	pid_t sender_pid;
};

typedef struct msg_recv_buf_s msg_recv_buf_t;

typedef enum ara_player_prio_s {
	PRIORITY_NORMAL = 0,
	PRIORITY_SYSTEM,
	PRIORITY_EMERGENCY
} ara_player_prio_t;

typedef enum ara_player_cmd_s {
	CMD_START = 0,
	CMD_STOP,
	CMD_PAUSE,
	CMD_RESUME
} ara_player_cmd_t;

typedef enum ara_player_stream_s {
	STREAM_LOCAL = 0,
	STREAM_HTTP_PROGRESSIVE,
	STREAM_HTTP_HLS,
	STREAM_HTTP_DASH,
	STREAM_HTTP_FILE
} ara_player_stream_t;

struct ara_player_msg_s {
	ara_player_prio_t priority;
	ara_player_stream_t streamType; // Local / Stream
	ara_player_cmd_t cmdType;		// start, stop, pause, resume
	char url[1024];

	// use the values below.
	audio_type_t audioType;
	audio_format_type_t pcmFormat;
	unsigned int channels;
	unsigned int sampleRate;
};

typedef struct ara_player_msg_s ara_player_msg_t;
typedef int (*StreamDataCB)(void *ptr, int type, size_t len, void *p);

typedef enum http_state_s {
	HTTP_DOWNLOAD_IDLE,
	HTTP_DOWNLOADING,
	HTTP_DOWNLOAD_PAUSE,
	HTTP_DOWNLOAD_COMPLETE
} http_state_t;

typedef enum request_type_s {
	HTTP_PROGRESSIVE = 0,
	HTTP_HLS_MANIFEST,
	HTTP_HLS_TS,
	HTTP_DASH_MPD,
	HTTP_DASH_SEGMENT
} request_type_t;

#if defined(MPLINUX)
#define LOG_ERR 1
#define LOG_WARNING 2
#define LOG_DEBUG 3
#define LOG_INFO 4
#endif

#endif
