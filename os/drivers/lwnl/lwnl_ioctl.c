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
#ifndef CONFIG_NET_NETMGR

#include <errno.h>
#include <sys/types.h>
#include <net/if.h>
#include <tinyara/lwnl/lwnl.h>
#include <tinyara/net/if/wifi.h>

static inline int _lwnl_message_fetch(const char *input, uint8_t **name, lwnl_req *req, void **data, uint32_t *data_len)
{
	if (!input) {
		return -1;
	}

	lwnl_msg *msg = (lwnl_msg *)input;
	*name = msg->name;
	*req = msg->req_type;
	*data = msg->data;
	*data_len = msg->data_len;

	return 0;
}

extern struct netdev *lwnl_get_dev(uint8_t *name);

int lwnl_message_handle(const char *msg, int msg_len)
{
	uint8_t *name = NULL;
	void *data = NULL;
	lwnl_req req = LWNL_REQ_UNKNOWN;
	uint32_t data_len = 0;
	lwnl_result_e res = LWNL_SUCCESS;

	int ret = _lwnl_message_fetch(msg, &name, &req, &data, &data_len);
	if (ret < 0) {
		ndbg("fetch lwnl message fail\n");
		return -ENOSYS;
	}

	struct netdev *dev = lwnl_get_dev(name);
	if (!dev) {
		ndbg("get lwnl netdev fail\n");
		return -ENOSYS;
	}

	// ToDo : if ethernet is supported then ops should be checked
	switch (req) {
	case LWNL_INIT:
	{
		res = ((struct trwifi_ops *)(dev->ops))->init(dev);
	}
	break;
	case LWNL_DEINIT:
	{
		res = ((struct trwifi_ops *)(dev->ops))->deinit(dev);
	}
	break;
	case LWNL_GET_INFO:
	{
		trwifi_info *info = (trwifi_info *)data;
		res = ((struct trwifi_ops *)(dev->ops))->get_info(dev, info);
	}
	break;
	case LWNL_SET_AUTOCONNECT:
	{
		uint8_t *check = (uint8_t *)data;
		res = ((struct trwifi_ops *)(dev->ops))->set_autoconnect(dev, *check);
	}
	break;
	case LWNL_START_STA:
	{
		res = ((struct trwifi_ops *)(dev->ops))->start_sta(dev);
	}
	break;
	case LWNL_CONNECT_AP:
	{
		trwifi_ap_config_s *config = (trwifi_ap_config_s *)data;
		res = ((struct trwifi_ops *)(dev->ops))->connect_ap(dev, config, NULL);
	}
	break;
	case LWNL_DISCONNECT_AP:
	{
		res = ((struct trwifi_ops *)(dev->ops))->disconnect_ap(dev, NULL);
	}
	break;
	case LWNL_START_SOFTAP:
	{
		trwifi_softap_config_s *config = (trwifi_softap_config_s *)data;
		res = ((struct trwifi_ops *)(dev->ops))->start_softap(dev, config);
	}
	break;
	case LWNL_STOP_SOFTAP:
	{
		res = ((struct trwifi_ops *)(dev->ops))->stop_softap(dev);
	}
	break;
	case LWNL_SCAN_AP:
	{
		trwifi_scan_config_s *config = (trwifi_scan_config_s *)data;
		res = ((struct trwifi_ops *)(dev->ops))->scan_ap(dev, config);
	}
	break;
	default:
		ret = -ENOSYS;
		break;
	}

	if (res != LWNL_SUCCESS) {
		ret = -ENOSYS;
	}

	return ret;
}

#endif // CONFIG_NET_NETMGR
