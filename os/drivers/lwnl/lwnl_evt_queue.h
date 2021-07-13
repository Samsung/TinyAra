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

#ifndef _LWNL_EVT_QUEUE_H__
#define _LWNL_EVT_QUEUE_H__

#include <tinyara/lwnl/lwnl.h>
#include <tinyara/fs/fs.h>

#define LWNL_NPOLLWAITERS 10

struct lwnl_event;
struct lwnl_queue;

void lwnl_queue_initialize(void);
int lwnl_add_listener(struct file *filep, lwnl_dev_type type);
int lwnl_remove_listener(struct file *filep);
int lwnl_get_event(struct file *filep, char *buf, int len);
int lwnl_check_queue(struct file *filep);
int lwnl_add_event(lwnl_cb_status type, void *buffer, uint32_t buf_len);

#endif // _LWNL_EVT_QUEUE_H__
