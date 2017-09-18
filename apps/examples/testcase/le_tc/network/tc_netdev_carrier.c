/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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

/// @file tc_netdev_carrier.c
/// @brief Test Case Example for netdev() API
#include <tinyara/net/netdev.h>
#include <net/lwip/netif.h>

#include "tc_internal.h"

/**
* @testcase				: tc_netdev_carrier_on_p
* @brief				:
* @scenario				:
* @apicovered			: netdev_carrier_on
* @precondition			:
* @postcondition		:
*/
static void tc_netdev_carrier_on_p(void)
{
	int ret;
	struct net_driver_s * dev;

	dev = (struct net_driver_s *)malloc(sizeof(struct net_driver_s));

	ret = netdev_carrier_on(dev);
	TC_ASSERT_EQ("netdev", ret, 0);
	TC_SUCCESS_RESULT();
}

/**
* @testcase				: tc_netdev_carrier_on_n
* @brief				:
* @scenario				:
* @apicovered			: netdev_carrier_on
* @precondition			:
* @postcondition		:
*/
static void tc_netdev_carrier_on_n(void)
{
	int ret;

	ret = netdev_carrier_on(NULL);
	TC_ASSERT_NEQ("netdev", ret, 0);
	TC_SUCCESS_RESULT();
}

/**
* @testcase				: tc_netdev_carrier_off_p
* @brief				:
* @scenario				:
* @apicovered			: netdev_carrier_off
* @precondition			:
* @postcondition		:
*/
static void tc_netdev_carrier_off_p(void)
{
	int ret;
	struct net_driver_s * dev;

	dev = (struct net_driver_s *)malloc(sizeof(struct net_driver_s));
	dev->d_flags = 4;

	ret = netdev_carrier_off(dev);
	TC_ASSERT_EQ("netdev", ret, 0);
	TC_SUCCESS_RESULT();
}

/**
* @testcase				: tc_netdev_carrier_off_n
* @brief				:
* @scenario				:
* @apicovered			: netdev_carrier_off
* @precondition			:
* @postcondition		:
*/
static void tc_netdev_carrier_off_n(void)
{
	int ret;
	struct net_driver_s * dev = NULL;

	dev = (struct net_driver_s *)malloc(sizeof(struct net_driver_s));
	dev->d_flags = 1;

	ret = netdev_carrier_off(dev);
	TC_ASSERT_NEQ("netdev", ret, 0);
	TC_SUCCESS_RESULT();
}

/**
* @testcase				: tc_netdev_count
* @brief				:
* @scenario				:
* @apicovered			: netdev_count
* @precondition			:
* @postcondition		:
*/
static void tc_netdev_count(void)
{
	int ndev;

	ndev = netdev_count();
	TC_SUCCESS_RESULT();
}

/**
* @testcase				: tc_netdev_ifdown
* @brief				:
* @scenario				:
* @apicovered			: netdev_ifdown
* @precondition			:
* @postcondition		:
*/
static int dummy_function (struct netif *dev)
{
	return 1;
}
static void tc_netdev_ifdown(void)
{
	struct netif *dev;
	dev = (struct netif *)malloc(sizeof(struct netif));

	dev->d_ifdown = dummy_function;
	netdev_ifdown (dev);
	TC_SUCCESS_RESULT();

}

/****************************************************************************
 * Name: netdev()
 ****************************************************************************/
int netdev_carrier_main(void)
{
	tc_netdev_carrier_on_p();
	tc_netdev_carrier_on_n();
	tc_netdev_carrier_off_p();
	tc_netdev_carrier_off_n();
	tc_netdev_count();
	tc_netdev_ifdown();
	return 0;
}
