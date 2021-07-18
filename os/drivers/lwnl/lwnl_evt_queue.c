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

#include <tinyara/config.h>
#include <stdlib.h>
#include <errno.h>
#include <net/if.h>
#include <pthread.h>
#include <debug.h>
#include <queue.h>
#include <tinyara/kmalloc.h>
#include <tinyara/net/if/wifi.h>
#include "lwnl_evt_queue.h"
#include "lwnl_log.h"

#define LWQ_LOCK								\
	do {										\
		sem_wait(&g_wm_sem);					\
	} while (0)
#define LWQ_UNLOCK								\
	do {										\
		sem_post(&g_wm_sem);					\
	} while (0)

#define CONTAINER_OF(ptr, type, member)									\
			((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))

#define LWQ_GET_EVT(ptr)\
	CONTAINER_OF(ptr, struct lwnl_event, entry)

#define LWQ_EVENT_HEADER (sizeof(lwnl_cb_status) + sizeof(uint32_t))

#define TAG "[LWQ]"

struct lwnl_event {
	lwnl_cb_data data;
	int8_t refs;
	sq_entry_t entry;
};
/*  every filep has own lwnl_filep. their relation is 1:1 */
struct lwnl_filep {
	struct file *filep; // file index
	/* If there is data to read in the queue then filep should call read() twice
	 * because a caller doesn't know how many data to read at first time.
	 * So filep read event type and data length at first then it read rest of data if
	 * data length isn't 0.
	 * A queue which belong to the filep has to store context to return data in second read().
	 * check_header stores the context of queue status because lwnl_filep is dedicated to filep.
	 * lwnl_event are shared to filep so it can't be used to store context.
	 * check_header 0: initial state. if there is data to send then assign 1
	 * check_header 1: there is data to send. */
	int8_t check_header;
	lwnl_dev_type type; // queue for wi-fi or ble
	sq_queue_t queue;
};

/* protect g_filep_list and g_connected*/
static sem_t g_wm_sem;
/* both data should be protected by LWQ_LOCK */
static struct lwnl_filep g_filep_list[LWNL_NPOLLWAITERS];
static struct sq_queue_t g_event_queue[LWNL_DEV_TYPE_MAX];
/* inserted event has to know how many fd wait */
static int g_connected[LWNL_DEV_TYPE_MAX] = {0, };
static int g_totalevt = 0; /*  debugging */

static int queue_size(sq_queue_t *queue)
{
	int size = 0;

	sq_entry_t *head = queue->head;
	sq_entry_t *tail = queue->tail;
	//sq_entry_t *cur = head;
	lldbg("[T%d] pk %p %p %p %p\n", getpid(),
		  queue, head, tail, (head != NULL ? head->flink : NULL));
	/* if (head) { */
	/* 	size = 1; */
	/* } */
	/* while (cur != tail) { */
	/* 	size++; */
	/* 	cur = head->flink; */
	/* } */
	return size;
}

static int _lwnl_update_filep(struct lwnl_event *evt)
{
	int check = 0;
	LWNL_LOGI(TAG, "%p\ttotal evt %d", evt, g_totalevt);
	LWQ_LOCK;
	lwnl_dev_type dtype = evt->data.status.type;
	for (int i = 0; i < LWNL_NPOLLWAITERS; i++) {
		if (g_filep_list[i].filep && g_filep_list[i].type == dtype) {
			check = 1;
			if (g_filep_list[i].queue.head == NULL) {
				g_filep_list[i].queue.head = evt;
				g_filep_list[i].queue.tail = evt;
			} else {
				g_filep_list[i].queue.tail = evt;
			}
		}
	}
	evt->refs = g_connected[dtype];
	if (check == 1) {
		g_totalevt++;
	}
	LWQ_UNLOCK;
	return 0;
}

/* this function is protected by LWQ_LOCK */
static int _lwnl_remove_event(struct lwnl_event *evt)
{
	LWNL_ENTER(TAG);
	evt->refs--;
	LWNL_LOGI(TAG, "refs(%d) (%p) (%p)",
			evt->refs, evt, evt->data.data);
	if (evt->refs > 0) {
		return 0;
	}

	LWNL_LOGI(TAG, "remove_event");
	lwnl_dev_type dtype = evt->data.status.type;
	sq_rem(evt->entry, &g_event_queue[dtype]);
	// it's not refered.
	if (evt->data.data) {
		kmm_free(evt->data.data);
		evt->data.data = NULL;
	}
	kmm_free(evt);
	g_totalevt--;

	return 0;
}

/**
 * API
 */
void lwnl_queue_initialize(void)
{
	LWNL_ENTER(TAG);
	LWQ_LOCK;
	for (int i = 0; i < LWNL_NPOLLWAITERS; i++) {
		g_filep_list[i].filep = NULL;
		g_filep_list[i].check_header = 0;
		sq_init(&g_filep_list[i].queue);
	}

	for (int i = 0; i < LWNL_DEV_TYPE_MAX; i++) {
		g_connected[i] = 0;
		sq_init(&g_event_queue[i]);
	}

	if (sem_init(&g_wm_sem, 0, 1) != 0) {
		LWNL_LOGE(TAG, "fail to init semaphore %d", errno);
	}
	LWQ_UNLOCK;
}

int lwnl_add_event(lwnl_cb_status type, void *buffer, uint32_t buf_len)
{
	LWNL_LOGI(TAG, "--> dev %d type %d buffer %p len (%d)",
			  type.type, type.evt, buffer, buf_len);
	struct lwnl_event *evt = (struct lwnl_event *)kmm_malloc(sizeof(struct lwnl_event));
	if (!evt) {
		LWNL_LOGE(TAG, "fail to alloc lwnl event");
		return -1;
	}
	evt->refs = 0;
	evt->data.status = type;

	if (buffer) {
		char *output = kmm_malloc(buf_len);
		if (!output) {
			LWNL_LOGE(TAG, "fail to alloc buffer");
			kmm_free(evt);
			return -3;
		}
		memcpy(output, buffer, buf_len);
		evt->data.data = output;
		evt->data.data_len = buf_len;
	} else {
		evt->data.data = NULL;
		evt->data.data_len = 0;
	}
	sq_addlast(evt->entry, &g_event_queue[type.type]);
	if (_lwnl_update_filep(evt) < 0) {
		LWNL_LOGE(TAG, "fail to update event");
		if (evt->data.data) {
			kmm_free(evt->data.data);
		}
		kmm_free(evt);
		return -2;
	}
	return 0;
}

int lwnl_get_event(struct file *filep, char *buf, int len)
{
	int written = 0;
	LWNL_ENTER(TAG);
	LWQ_LOCK;
	struct lwnl_filep *fp = filep->f_priv;
	if (!fp) {
		LWQ_UNLOCK;
		LWNL_LOGE(TAG, "filep doens't have fp");
		return -1;
	}

	if (sq_empty(&fp->fp)) {
		LWQ_UNLOCK;
		LWNL_LOGE(TAG, "filep doesn't have item");
		return 0;
	}

	struct lwnl_event *evt = LWQ_GET_EVT(sq_peek(&fp->fp));
	LWNL_LOGI(TAG, "event %p check %d fp %p", evt, fp->check_header, filep);
	if (fp->check_header == 0) {
		if (len < LWQ_EVENT_HEADER) {
			LWQ_UNLOCK;
			LWNL_LOGE(TAG, "buffer length is less than header");
			return -1;
		}
		memcpy(buf, &evt->data.status, sizeof(lwnl_cb_status));
		buf += sizeof(lwnl_cb_status);
		memcpy(buf, &evt->data.data_len, sizeof(uint32_t));
		written = LWQ_EVENT_HEADER;
		if (evt->data.data_len > 0) {
			fp->check_header = 1;
			LWQ_UNLOCK;
			return written;
		}
	} else {
		if (len < evt->data.data_len) {
			LWQ_UNLOCK;
			LWNL_LOGE(TAG, "buffer length (%d) is less than data length (%d)", len, evt->data.data_len);
			return -1;
		}
		memcpy(buf, evt->data.data, evt->data.data_len);
		fp->check_header = 0;
		written = evt->data.data_len;
	}
	LWNL_LOGI(TAG, " remove_item (%p) in fp (%p)", evt, filep);
	lldbg("[T%d] mid fp size %d\n", getpid(), queue_size(&fp->fp));
	sq_remfirst(&fp->fp);
	_lwnl_remove_event(evt);
	lldbg("[T%d] remain size fp %p (%d)\n", getpid(), filep, queue_size(&fp->fp));

	LWQ_UNLOCK;
	return written;
}

int lwnl_add_listener(struct file *filep, lwnl_dev_type type)
{
	LWNL_LOGI(TAG, "--> fp (%p) type (%d)", filep, type);
	LWQ_LOCK;
	for (int i = 0; i < LWNL_NPOLLWAITERS; i++) {
		if (!g_filep_list[i].filep) {
			LWNL_LOGI(TAG, "add fp %p to idx %d %p", filep, i);
			g_filep_list[i].filep = filep;
			filep->f_priv = (void *)&g_filep_list[i];
			g_filep_list[i].type = type;
			g_connected[type]++;
			LWQ_UNLOCK;
			return 0;
		}
	}
	LWQ_UNLOCK;
	LWNL_LOGE(TAG, "no available listener slot");
	return -1;
}

lwnl_dev_type lwnl_get_dev_type(struct file *filep)
{
	if (filep->f_priv) {
		struct lwnl_filep *queue = (struct lwnl_filep *)filep->f_priv;
		return queue->type;
	}
	return LWNL_DEV_TYPE_MAX; // unknown
}
int lwnl_remove_listener(struct file *filep)
{
	LWNL_LOGI(TAG, "remove listener filep %p %d %d",
			filep, g_totalevt, LWNL_NPOLLWAITERS);
	LWQ_LOCK;
	struct lwnl_filep *queue = (struct lwnl_filep *)filep->f_priv;
	if (!queue) {
		// some socket doens't bind event listener.
		// so it can cause overhead.
		LWQ_UNLOCK;
		return 0;
	}

	struct lwnl_event *evt = NULL;
	while ((evt = LWQ_GET_EVT(sq_remfirst(&queue->queue))) != NULL) {
		LWNL_LOGI(TAG, "remove evt %p in fp %p", evt, filep);
		_lwnl_remove_event(evt);
		lldbg("[T%d] fp %p size %d\n", filep, queue_size(&queue->queue));
	}
	g_connected[queue->type]--;
	queue->check_header = 0;
	filep->f_priv = queue->filep = NULL;

	LWQ_UNLOCK;
	return 0;
}

/* Description: if filep has event then it return 1 else return 0 */
int lwnl_check_queue(struct file *filep)
{
	int res = 0;
	LWNL_ENTER(TAG);
	LWQ_LOCK;
	struct lwnl_filep *queue = (struct lwnl_filep *)filep->f_priv;
	if (!queue) {
		lldbg("[T%d] queue is null\n", getpid(), queue);
		goto done;
	}
	if (!sq_empty(&queue->queue)) {
		res = 1;
	}
done:
	lldbg("[T%d] que status(%d) %d\n", getpid(), res, queue_size(&queue->queue));
	LWQ_UNLOCK;
	return res;
}
