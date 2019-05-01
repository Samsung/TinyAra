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
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <debug.h>
#include <tinyara/heapinfo_drv.h>
#include <tinyara/mm/mm.h>
#include <tinyara/fs/fs.h>
#include <tinyara/fs/ioctl.h>

#ifdef CONFIG_MM_KERNEL_HEAP
extern struct mm_heap_s g_kmmheap;
#endif
/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int heapinfo_ioctl(FAR struct file *filep, int cmd, unsigned long arg);
static ssize_t heapinfo_read(FAR struct file *filep, FAR char *buffer, size_t len);
static ssize_t heapinfo_write(FAR struct file *filep, FAR const char *buffer, size_t len);

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct heapinfo_dev_s {
	mqd_t mqdes;	   /* A mqueue descriptor */
};

static const struct file_operations heapinfo_fops = {
	0,                          /* open */
	0,                          /* close */
	heapinfo_read,               /* read */
	heapinfo_write,              /* write */
	0,                          /* seek */
	heapinfo_ioctl               /* ioctl */
#ifndef CONFIG_DISABLE_POLL
	, 0                         /* poll */
#endif
};
/****************************************************************************
 * Private Functions
 ****************************************************************************/
static ssize_t heapinfo_read(FAR struct file *filep, FAR char *buffer, size_t len)
{
	return 0;
}

static ssize_t heapinfo_write(FAR struct file *filep, FAR const char *buffer, size_t len)
{
	return 0;
}

/************************************************************************************
 * Name: heapinfo_ioctl
 *
 * Description: The ioctl method for heapinfo.
 *
 ************************************************************************************/
static int heapinfo_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	int ret = ERROR;
#ifdef CONFIG_MM_KERNEL_HEAP
	heapinfo_option_t *option;
#endif

	/* Handle built-in ioctl commands */

	switch (cmd) {
#ifdef CONFIG_MM_KERNEL_HEAP
	case HEAPINFOIOC_PARSE:
		option = (heapinfo_option_t *)arg;
		heapinfo_parse(&g_kmmheap, option->mode, option->pid);
		ret = OK;
		break;
#if CONFIG_MM_REGIONS > 1
	case HEAPINFOIOC_NREGION:
		ret = g_kmmheap.mm_nregions;
		break;
#endif
#endif
	default:
		break;
	}
	return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: heapinfo_drv_register
 *
 * Description:
 *   Register heapinfo driver path, HEAPINFO_DRVPATH
 *
 ****************************************************************************/

void heapinfo_drv_register(void)
{
	(void)register_driver(HEAPINFO_DRVPATH, &heapinfo_fops, 0666, NULL);
}
