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

#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <poll.h>
#include <errno.h>
#include <debug.h>
#include <net/if.h>
#include <tinyara/fs/fs.h>
#include <tinyara/lwnl/lwnl.h>
#include "lwnl_evt_queue.h"
#include "lwnl_log.h"

#define TAG "[LWNL]"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWNLDEV_LOCK(upper)							\
	do {											\
		int lock_ret = sem_wait(&upper->exclsem);	\
		if (lock_ret < 0) {							\
			LWNL_LOGE(TAG, "fail to lock");			\
			return lock_ret;						\
		}											\
	} while (0)

#define LWNLDEV_UNLOCK(upper)					\
	do {										\
		sem_post(&upper->exclsem);				\
	} while (0)


/****************************************************************************
 * Private Types
 ****************************************************************************/
struct lwnl_open_s {

	/* The following will be true if we are closing */
	volatile bool io_closing;

#ifndef CONFIG_DISABLE_POLL
	/*
	 * The following is a list if poll structures of threads waiting
	 * for driver events
	 */
	FAR struct pollfd *io_fds[LWNL_NPOLLWAITERS];
#endif
};

struct lwnl_upperhalf_s {
	uint8_t crefs;
	volatile bool started;
	sem_t exclsem;
	void *lower; /* Arch-specific operations */
	struct lwnl_open_s ln_open;
	//pkbuild struct lwnl_queue *queue;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int lwnl_open(struct file *filep);
static int lwnl_close(struct file *filep);
static ssize_t lwnl_read(struct file *filep, char *buffer, size_t len);
static ssize_t lwnl_write(struct file *filep, const char *buffer, size_t len);
static int lwnl_ioctl(struct file *filep, int cmd, unsigned long arg);
#ifndef CONFIG_DISABLE_POLL
static int lwnl_poll(FAR struct file *filep, FAR struct pollfd *fds, bool setup);
#endif

#ifndef CONFIG_NET_NETMGR
extern int lwnl_message_handle(const char *msg, int msg_len);
extern void lwnl_initialize_dev(void);
#else
extern int netdev_req_handle(const char *msg, size_t msg_len);
#endif

#ifdef CONFIG_BLE_MANAGER
extern int bledev_req_handle(const char *msg, size_t msg_len);
#endif
/****************************************************************************
 * Private Data
 ****************************************************************************/
static const struct file_operations g_lwnl_fops = {
	lwnl_open,                                          /* open */
	lwnl_close,                                         /* close */
	lwnl_read,                                          /* read */
	lwnl_write,                                         /* write */
	0,                                                       /* seek */
	lwnl_ioctl                                          /* ioctl */
#ifndef CONFIG_DISABLE_POLL
	, lwnl_poll                                                      /* poll */
#endif
};
struct lwnl_upperhalf_s *g_lwnl_upper = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int lwnl_open(struct file *filep)
{
	LWNL_ENTER(TAG);
	struct inode *inode = filep->f_inode;
	struct lwnl_upperhalf_s *upper = inode->i_private;

	LWNLDEV_LOCK(upper);
	/*
	 * crefs type is uint8 so overflow might happen.
	 * However there are a few modules which use lwnl.
	 * So checking overflow is not needed now.
	 */
	upper->crefs++;
	/* lwnl queue will be assigned if fd calls bind()
	 * Therefore it's safe to initialize f_priv to check whether
	 * queue is assigned or not*/
	filep->f_priv = NULL;
	LWNLDEV_UNLOCK(upper);

	LWNL_LEAVE(TAG);

	return OK;
}

static int lwnl_close(struct file *filep)
{
	LWNL_ENTER(TAG);
	struct inode *inode = filep->f_inode;
	struct lwnl_upperhalf_s *upper = inode->i_private;
	int ret = OK;
#ifndef CONFIG_DISABLE_POLL
	FAR struct lwnl_open_s *ln_open = &upper->ln_open;
	int waiter_idx;
#endif

	LWNLDEV_LOCK(upper);
	if (upper->crefs > 0) {
		upper->crefs--;
	} else {
		/*
		 * lwnl driver doesn't have resources to free. So it doesn't care
		 * upper->cres == 0 case
		 */
		LWNL_LOGE(TAG, "critical error");
		ret = -EBADF;
	}

#ifndef CONFIG_DISABLE_POLL

	/*
	 * Check if this file is registered in a list of waiters for polling.
	 * If it is, the used slot should be cleared.
	 * Otherwise, an invalid pollfd remains in a list and this slot is not available forever.
	 */
	for (waiter_idx = 0; waiter_idx < LWNL_NPOLLWAITERS; waiter_idx++) {
		struct pollfd *fds = ln_open->io_fds[waiter_idx];
		if (fds && (FAR struct file *)fds->filep == filep) {
			ln_open->io_fds[waiter_idx] = NULL;
		}
	}
#endif
	LWNLDEV_UNLOCK(upper);

	int res = lwnl_remove_listener(filep);
	if (res < 0) {
		LWNL_LOGE(TAG, "remove listener fail");
		ret = -ENOSYS;
	}

	LWNL_LEAVE(TAG);
	return ret;
}

static ssize_t lwnl_read(struct file *filep, char *buffer, size_t len)
{
	LWNL_ENTER(TAG);

	int res = lwnl_get_event(filep, buffer, len);

	// todo_net : convert res to vfs error style?
	LWNL_LEAVE(TAG);
	return res;
}

static ssize_t lwnl_write(struct file *filep, const char *buffer, size_t len)
{
	LWNL_ENTER(TAG);
#ifdef CONFIG_NET_NETMGR
	int ret = netdev_req_handle(buffer, len);
#ifdef CONFIG_BLE_MANAGER
	if (ret == -ENOSYS) {
		ret = bledev_req_handle(buffer, len);
	}
#endif
#else
	int ret = lwnl_message_handle(buffer, len);
#endif
	LWNL_LEAVE(TAG);
	if (ret < 0) {
		LWNL_LOGE(TAG, "handle request fail");
		return -1;
	}
	return len;
}

static int lwnl_ioctl(struct file *filep, int cmd, unsigned long arg)
{
	LWNL_ENTER(TAG);
	struct sockaddr_lwnl *addr = (struct sockaddr_lwnl *)arg;
	int res = lwnl_add_listener(filep, addr->dev_type);
	if (res < 0) {
		LWNL_LOGE(TAG, "add listener fail");
		res = -EBADF;
	}
	LWNL_LEAVE(TAG);
	return res;
}

static int lwnl_poll(FAR struct file *filep, FAR struct pollfd *fds, bool setup)
{
	lldbg("[pkbuild] [T%d] --> poll (%d) \n", getpid(), setup);
	LWNL_ENTER(TAG);

	FAR struct inode *inode;
	FAR struct lwnl_upperhalf_s *upper;
	FAR struct lwnl_open_s *ln_open;
	int ret = 0;

	DEBUGASSERT(filep && filep->f_inode);
	inode = filep->f_inode;
	DEBUGASSERT(inode->i_private);
	upper  = (FAR struct lwnl_upperhalf_s *)inode->i_private;
	ln_open = &upper->ln_open;

	/* Are we setting up the poll? Or tearing it down? */
	if (setup) {
		/*  Check if any requested events are already in effect */
		if (fds->events & POLLIN) {
			int nready = lwnl_check_queue(filep);
			if (nready > 0) {
				fds->revents |= (fds->events & POLLIN);
				sem_post(fds->sem);
				return 0;
			}
		}

		/*
		 * This is a request to set up the poll. Find an available
		 * slot for the poll structure reference
		 */
		/* Get exclusive access to the driver structure */
		LWNLDEV_LOCK(upper);
		int i = 0;
		for (; i < LWNL_NPOLLWAITERS; i++) {
			/* Find an available slot */
			if (!ln_open->io_fds[i]) {
				/* Bind the poll structure and this slot */
				ln_open->io_fds[i] = fds;
				fds->priv = &ln_open->io_fds[i];
				fds->filep = (void *)filep;
				lldbg("[pkbuild] [T%d] set %p %p\n", getpid(), fds, fds->sem);
				break;
			}
		}
		LWNLDEV_UNLOCK(upper);

		if (i >= LWNL_NPOLLWAITERS) {
			LWNL_LOGE(TAG, "Too many poll waiters");
			fds->priv = NULL;
			ret       = -EBUSY;
			goto errout_with_dusem;
		}

		/* Call lwnl_pollscan again: there could have been events between
		   the last scan (without us on the list) and putting us on the list! */
		if (fds->events & POLLIN) {
			int nready = lwnl_check_queue(filep);
			if (nready > 0) {
				fds->revents |= (fds->events & POLLIN);
				sem_post(fds->sem);
				return 0;
			}
		}
	} else if (fds->priv) {
		/* This is a request to tear down the poll. */
		FAR struct pollfd **slot = (FAR struct pollfd **)fds->priv;

		/* Remove all memory of the poll setup */
		*slot = NULL;
		fds->priv = NULL;
	}

errout_with_dusem:
	return ret;
}

/*
 * Public APIs
 */
int lwnl_register(struct lwnl_lowerhalf_s *dev)
{
	LWNL_ENTER(TAG);
	struct lwnl_upperhalf_s *upper = NULL;
	int ret;

#ifndef CONFIG_NET_NETMGR
	lwnl_initialize_dev();
#endif

	upper = (struct lwnl_upperhalf_s *)kmm_zalloc(sizeof(struct lwnl_upperhalf_s));
	if (!upper) {
		LWNL_LOGE(TAG, "fail to alloc memory");
		return -ENOMEM;
	}

	for (int i = 0; i < LWNL_NPOLLWAITERS; i++) {
		upper->ln_open.io_fds[i] = NULL;
	}

	sem_init(&upper->exclsem, 0, 1);
	upper->lower = (void *)dev;
	dev->parent = upper;
	g_lwnl_upper = upper;

	lwnl_queue_initialize();

	ret = register_driver(LWNL_PATH, &g_lwnl_fops, 0666, upper);
	if (ret < 0) {
		LWNL_LOGE(TAG, "fail to register lwnl driver");
		goto errout_with_priv;
	}

	LWNL_LEAVE(TAG);

	return OK;

errout_with_priv:
	sem_destroy(&upper->exclsem);
	kmm_free(upper);
	return ret;
}

int lwnl_unregister(struct lwnl_lowerhalf_s *dev)
{
	if (!dev) {
		LWNL_LOGE(TAG, "dev is null");
		return -1;
	}

	struct lwnl_upperhalf_s *upper =  dev->parent;
	if (!upper) {
		LWNL_LOGE(TAG, "upper is null");
		return -2;
	}

	sem_destroy(&upper->exclsem);
	kmm_free(upper);

	return 0;
}

int lwnl_postmsg(lwnl_dev_type dev, uint32_t evt, void *buffer, uint32_t buf_len)
{
	if (!g_lwnl_upper) {
		return -1;
	}
	lwnl_cb_status cb = {dev, evt};
	int res = lwnl_add_event(cb, buffer, buf_len);
	if (res < 0) {
		LWNL_LOGE(TAG, "fail to add the event %d", res);
		return res;
	}

	LWNLDEV_LOCK(g_lwnl_upper);
	for (int i = 0; i < LWNL_NPOLLWAITERS; i++) {
		struct pollfd *fds = g_lwnl_upper->ln_open.io_fds[i];
		if (fds && (lwnl_get_dev_type(fds->filep) == dev)) {
			fds->revents |= (fds->events & POLLIN);
			if (fds->revents != 0) {
				lldbg("[pkbuild] [T%d] post %p %p\n", getpid(), fds, fds->sem);
				int tres = sem_post(fds->sem);
				if (tres != 0) {
					int err_no = get_errno();
					lldbg("[pkbuild] sem post fail %d\n", err_no);
				}
			}
		}
	}
	LWNLDEV_UNLOCK(g_lwnl_upper);
	return 0;
}
