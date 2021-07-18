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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#include "netstack.h"
#include <tinyara/net/netlog.h>

#define TAG "[NETMGR]"

static int _create_netlink(int type, int protocol)
{
	// to do: filter type and protocol
	(void)type;
	(void)protocol;

	int fd = open(LWNL_PATH, O_RDWR);
	if (fd < 0) {
		NET_LOGE(TAG, "open netlink dev fail\n");
		return -1;
	}
	return fd;
}

static int netlink_socket(int domain, int type, int protocol)
{
	return _create_netlink(type, protocol);
}

// allocate network event listen
static int netlink_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
	(void)namelen;
	return ioctl(s, SIOCSLWNLEVT, name);
}

struct netstack_ops g_netlink_stack_ops = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	netlink_socket,
	netlink_bind,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
	NULL,
#ifdef CONFIG_NET_ROUTE
	NULL,
	NULL,
#endif
#ifdef CONFIG_NET_NETMON
	NULL,
#endif
};

struct netstack g_netlink_stack = {&g_netlink_stack_ops, NULL};

struct netstack *get_netstack_netlink(void)
{
	return &g_netlink_stack;
}
