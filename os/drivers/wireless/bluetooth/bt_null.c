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
 * drivers/wireless/bluetooth/bt_null.c
 * UART based Bluetooth driver
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>

#include <tinyara/bluetooth/bt_driver.h>

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/bluetooth/iob/iob.h>
#include <tinyara/bluetooth/bluetooth.h>
#include <tinyara/bluetooth/bt_hci.h>
#include <tinyara/bluetooth/bt_null.h>
#include <tinyara/bluetooth/bt_buf.h>

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void btnull_format_cmdcomplete(FAR struct bt_buf_s *buf, uint16_t opcode);
static void btnull_format_bdaddr_rsp(FAR struct bt_buf_s *buf, uint16_t opcode);

static int btnull_open(FAR const struct bt_driver_s *dev);
static int btnull_send(FAR const struct bt_driver_s *dev, FAR struct bt_buf_s *buf);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct bt_driver_s g_bt_null = {
	0,							/* head_reserve */
	btnull_open,				/* open */
	btnull_send					/* send */
};

static const bt_addr_t g_bt_addr = {
	{0xfe, 0xed, 0xb0, 0x0b, 0xfa, 0xce}
};

static uint8_t local_name[HCI_MAX_NAME_LENGTH];

/* scan parameter */
FAR struct bt_hci_cp_le_set_scan_params_s g_scan_parms;
FAR struct bt_hci_cp_le_set_scan_enable_s g_scan_enable;

struct bt_hci_rp_scan_enable_status_s {
	uint8_t status;
};

uint8_t g_ad_data[31] = { 0x02, 0x01, 0x1a, 0x1b, 0xff, 0x75, 0x00, 0x42, 0x04, 0x01, 0x20, 0x6f,
						  0x19, 0x0f, 0x00, 0x02, 0x01, 0x37, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
						  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
						};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void btnull_format_cmdcomplete(FAR struct bt_buf_s *buf, uint16_t opcode)
{
	struct bt_hci_evt_hdr_s evt;
	struct hci_evt_cmd_complete_s cmd;
	FAR uint8_t *data = buf->frame->io_data;
	int ndx;
	int len;

	/* Minimal setup for the command complete event */

	len = sizeof(struct bt_hci_evt_hdr_s) + sizeof(struct hci_evt_cmd_complete_s);
	ndx = 0;

	evt.evt = BT_HCI_EVT_CMD_COMPLETE;
	evt.len = len;
	memcpy(&data[ndx], &evt, sizeof(struct bt_hci_evt_hdr_s));
	ndx += sizeof(struct bt_hci_evt_hdr_s);

	cmd.ncmd = 1;
	cmd.opcode = opcode;
	memcpy(&data[ndx], &cmd, sizeof(struct hci_evt_cmd_complete_s));
	ndx += sizeof(struct hci_evt_cmd_complete_s);

	buf->frame->io_len = len;
	buf->len = len;
}

static void btnull_format_local_features_rsp(FAR struct bt_buf_s *buf, uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features_s features;
	FAR uint8_t *data = buf->frame->io_data;
	int ndx;
	int len;

	/* Return local features */

	btnull_format_cmdcomplete(buf, opcode);

	len = buf->len + sizeof(struct bt_hci_rp_le_read_local_features_s);
	ndx = buf->len;

	memset(&features, 0, sizeof(struct bt_hci_rp_le_read_local_features_s));
	if (opcode == BT_HCI_OP_READ_LOCAL_FEATURES) {
		features.features[4] = BT_LMP_LE;
	} else {					/* if opcode == BT_HCI_OP_LE_READ_LOCAL_FEATURES */

		features.features[0] = BT_HCI_LE_ENCRYPTION;
	}

	memcpy(&data[ndx], &features, sizeof(struct bt_hci_rp_le_read_local_features_s));
	ndx += sizeof(struct bt_hci_rp_le_read_local_features_s);

	buf->frame->io_len = len;
	buf->len = len;
}

static void btnull_format_bdaddr_rsp(FAR struct bt_buf_s *buf, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr_s bdaddr;
	FAR uint8_t *data = buf->frame->io_data;
	int ndx;
	int len;

	btnull_format_cmdcomplete(buf, opcode);

	len = buf->len + sizeof(struct bt_hci_rp_read_bd_addr_s);
	ndx = buf->len;

	BLUETOOTH_ADDRCOPY(bdaddr.bdaddr.val, g_bt_addr.val);
	bdaddr.status = 0;
	memcpy(&data[ndx], &bdaddr, sizeof(struct bt_hci_rp_read_bd_addr_s));
	ndx += sizeof(struct bt_hci_rp_read_bd_addr_s);

	buf->frame->io_len = len;
	buf->len = len;
}

static void btnull_format_buffersize_rsp(FAR struct bt_buf_s *buf, uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size_s bufsize;
	FAR uint8_t *data = buf->frame->io_data;
	int ndx;
	int len;

	btnull_format_cmdcomplete(buf, opcode);

	len = buf->len + sizeof(struct bt_hci_rp_le_read_buffer_size_s);
	ndx = buf->len;

	bufsize.status = 0;
	bufsize.le_max_len = BLUETOOTH_MAX_FRAMELEN;
	bufsize.le_max_num = 1;
	memcpy(&data[ndx], &bufsize, sizeof(struct bt_hci_rp_le_read_buffer_size_s));
	ndx += sizeof(struct bt_hci_rp_le_read_buffer_size_s);

	buf->frame->io_len = len;
	buf->len = len;
}

static void btnull_format_read_local_name_rsp(FAR struct bt_buf_s *buf, uint16_t opcode)
{
	struct hci_rp_read_local_name_s lname;
	FAR uint8_t *data = buf->frame->io_data;
	int ndx;
	int len;

	btnull_format_cmdcomplete(buf, opcode);

	len = buf->len + sizeof(struct hci_rp_read_local_name_s);
	ndx = buf->len;

	memset(&lname, 0, sizeof(struct hci_rp_read_local_name_s));
	lname.status = 0;
	memcpy(lname.local_name, local_name, HCI_MAX_NAME_LENGTH);

	memcpy(&data[ndx], &lname, sizeof(struct hci_rp_read_local_name_s));
	ndx += sizeof(struct hci_rp_read_local_name_s);

	buf->frame->io_len = len;
	buf->len = len;
}

static void btnull_process_write_local_name_req(FAR struct bt_buf_s *buf)
{
	FAR struct bt_hci_cmd_hdr_s *hdr;
	FAR struct hci_cp_write_local_name_s *info;

	if (buf->type == BT_CMD) {
		hdr = (FAR void *)buf->data;
		uint16_t opcode = hdr->opcode;

		nvdbg("CMD: %04x\n", opcode);

		info = bt_buf_consume(buf, sizeof(struct bt_hci_cmd_hdr_s));

		nvdbg("local_name received: %s\n", info->local_name);

		memset(local_name, 0, HCI_MAX_NAME_LENGTH);
		memcpy(local_name, info->local_name, HCI_MAX_NAME_LENGTH);
	}
}

static void btnull_process_set_scan_params_req(FAR struct bt_buf_s *buf)
{
	FAR struct bt_hci_cmd_hdr_s *hdr;
	FAR struct bt_hci_cp_le_set_scan_params_s *set_param;

	nvdbg("BT_HCI_OP_LE_SET_SCAN_PARAMS request is received");

	if (buf->type == BT_CMD) {
		hdr = (FAR void *)buf->data;
		uint16_t opcode = hdr->opcode;

		nvdbg("CMD: %04x\n", opcode);

		set_param = bt_buf_consume(buf, sizeof(struct bt_hci_cmd_hdr_s));

		memset(&g_scan_parms, 0, sizeof(g_scan_parms));
		g_scan_parms.scan_type = set_param->scan_type;
		g_scan_parms.interval = set_param->interval;
		g_scan_parms.window = set_param->window;
		g_scan_parms.addr_type = set_param->addr_type;
		g_scan_parms.filter_policy = set_param->filter_policy;

		nvdbg("scan_type is: [%d]", g_scan_parms.scan_type);
	}
}

static void btnull_process_scan_enable_req(FAR struct bt_buf_s *buf)
{
	FAR struct bt_hci_cmd_hdr_s *hdr;
	FAR struct bt_hci_cp_le_set_scan_enable_s *scan_enable;

	nvdbg("BT_HCI_OP_LE_SET_SCAN_ENABLE request is received");

	if (buf->type == BT_CMD) {
		hdr = (FAR void *)buf->data;
		uint16_t opcode = hdr->opcode;

		nvdbg("CMD: %04x\n", opcode);

		scan_enable = bt_buf_consume(buf, sizeof(struct bt_hci_cmd_hdr_s));
		g_scan_enable.enable = scan_enable->enable;
		g_scan_enable.filter_dup = scan_enable->filter_dup;

		nvdbg("scan enable: [%d]", g_scan_enable.enable);
	}
}

static void btnull_format_scan_status_rsp(FAR struct bt_buf_s *buf, uint16_t opcode)
{
	struct bt_hci_rp_scan_enable_status_s scan_status;
	FAR uint8_t *data = buf->frame->io_data;
	int ndx;
	int len;

	nvdbg("scan_status_rsp");

	btnull_format_cmdcomplete(buf, opcode);

	len = buf->len + sizeof(struct bt_hci_rp_scan_enable_status_s);
	ndx = buf->len;

	scan_status.status = 0;
	memcpy(&data[ndx], &scan_status, sizeof(struct bt_hci_rp_scan_enable_status_s));
	ndx += sizeof(struct bt_hci_rp_scan_enable_status_s);

	buf->frame->io_len = len;
	buf->len = len;
}

static void btnull_format_le_meta_event(FAR struct bt_buf_s *buf, uint16_t opcode)
{
	struct bt_hci_evt_hdr_s evt;
	struct bt_hci_evt_le_meta_event_s meta_evt;
	FAR uint8_t *data = buf->frame->io_data;
	int ndx;
	int len;

	nvdbg("le_meta_event");

	/* Minimal setup for the le meta event */

	len = sizeof(struct bt_hci_evt_hdr_s) + sizeof(struct bt_hci_evt_le_meta_event_s);
	ndx = 0;

	evt.evt = BT_HCI_EVT_LE_META_EVENT;
	evt.len = len;
	memcpy(&data[ndx], &evt, sizeof(struct bt_hci_evt_hdr_s));
	ndx += sizeof(struct bt_hci_evt_hdr_s);

	meta_evt.subevent = BT_HCI_EVT_LE_ADVERTISING_REPORT;
	memcpy(&data[ndx], &meta_evt, sizeof(struct bt_hci_evt_le_meta_event_s));
	ndx += sizeof(struct bt_hci_evt_le_meta_event_s);

	buf->frame->io_len = len;
	buf->len = len;
}

static void btnull_format_scan_result_rsp(FAR struct bt_buf_s *buf, uint16_t opcode)
{
	struct bt_hci_ev_le_advertising_info_s adv_info;
	FAR uint8_t *data = buf->frame->io_data;
	int ndx;
	int len;
	uint8_t num_reports;
	uint8_t rssi = 0x9f;

	nvdbg("scan_result_rsp");

	btnull_format_le_meta_event(buf, opcode);

	len = buf->len + sizeof(num_reports) + sizeof(struct bt_hci_ev_le_advertising_info_s) + 31 + 1;
	ndx = buf->len;

	/* num report */
	num_reports = 1;
	memcpy(&data[ndx], &num_reports, sizeof(uint8_t));
	ndx += sizeof(uint8_t);

	/* adv_info */
	memset(&adv_info, 0, sizeof(struct bt_hci_ev_le_advertising_info_s));
	adv_info.evt_type = 0;
	BLUETOOTH_ADDRCOPY(adv_info.addr.val, g_bt_addr.val);
	adv_info.length = 31;
	memcpy(&data[ndx], &adv_info, sizeof(struct bt_hci_ev_le_advertising_info_s));
	ndx += sizeof(struct bt_hci_ev_le_advertising_info_s);

	/* data copy */
	memcpy(&data[ndx], &g_ad_data, 31);
	ndx += 31;

	/* rssi */
	memcpy(&data[ndx], rssi, sizeof(uint8_t));

	buf->frame->io_len = len;
	buf->len = len;
}

static int btnull_send(FAR const struct bt_driver_s *dev, FAR struct bt_buf_s *buf)
{
	nvdbg("Bit bucket: length %d\n", (int)buf->len);

	/* Is the Bluetooth stack waiting for an event? */

	if (buf->type == BT_CMD) {
		FAR struct bt_hci_cmd_hdr_s *hdr = (FAR void *)buf->data;
		FAR struct bt_buf_s *outbuf;
		uint16_t opcode = hdr->opcode;

		nvdbg("CMD: %04x\n", opcode);

		outbuf = bt_buf_alloc(BT_EVT, NULL, 0);
		if (outbuf == NULL) {
			ndbg("ERROR: Failed to allocate buffer\n");
			return -ENOMEM;
		}

		switch (opcode) {
		case BT_HCI_OP_READ_LOCAL_FEATURES:
		case BT_HCI_OP_LE_READ_LOCAL_FEATURES:
			btnull_format_local_features_rsp(outbuf, opcode);
			break;

		case BT_HCI_OP_READ_BD_ADDR:
			btnull_format_bdaddr_rsp(outbuf, opcode);
			break;

		case BT_HCI_OP_LE_READ_BUFFER_SIZE:
			btnull_format_buffersize_rsp(outbuf, opcode);
			break;

		case BT_HCI_OP_WRITE_LOCAL_NAME:
			btnull_process_write_local_name_req(buf);
			btnull_format_cmdcomplete(outbuf, opcode);
			break;

		case BT_HCI_OP_READ_LOCAL_NAME:
			btnull_format_read_local_name_rsp(outbuf, opcode);
			break;

		case BT_HCI_OP_LE_SET_SCAN_PARAMS:
			btnull_process_set_scan_params_req(buf);
			btnull_format_cmdcomplete(outbuf, opcode);

		case BT_HCI_OP_LE_SET_SCAN_ENABLE:
			btnull_process_scan_enable_req(buf);
			btnull_format_scan_status_rsp(outbuf, opcode);
			btnull_format_scan_result_rsp(outbuf, opcode);

		default:
			btnull_format_cmdcomplete(outbuf, opcode);
			break;
		}

		nvdbg("Send CMD complete event\n");

		bt_hci_receive(outbuf);
	}

	return buf->len;
}

static int btnull_open(FAR const struct bt_driver_s *dev)
{
	return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: btnull_register
 *
 * Description:
 *   Register the NULL Bluetooth stack with the Bluetooth stack
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero is returned on success; a negated errno value is returned on any
 *   failure.
 *
 ****************************************************************************/

int btnull_register(void)
{
	/* Register the driver with the Bluetooth stack */

	return bt_dev_register(&g_bt_null);
}
