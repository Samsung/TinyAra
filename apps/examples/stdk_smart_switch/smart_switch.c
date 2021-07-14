/* ***************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
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

//for implementing main features of IoT device
#include <stdbool.h>
#include <stdio.h>
#include "st_dev.h"
#include "device_control.h"
#include "device_info.h"
#include "onboarding_config.h"
#include "iot_os_util.h"

static const char *TAG = "SLamp";
#define SL_LOGE(tag, format, ...) do {printf("%s ", tag); printf(format"\n", ##__VA_ARGS__); } while (0)
#define SL_LOGI(tag, format, ...) do {printf("%s ", tag); printf(format"\n", ##__VA_ARGS__); } while (0)

enum smartlamp_switch_onoff_state {
	SMARTLAMP_SWITCH_OFF = 0,
	SMARTLAMP_SWITCH_ON = 1,
};

static int smartlamp_switch_state = SMARTLAMP_SWITCH_ON;

static iot_status_t g_iot_status;

static int noti_led_mode = LED_ANIMATION_MODE_IDLE;

/* TODO: Need 'get_ctx' function (get ctx from cap_handle) */
IOT_CTX* ctx = NULL;

static void change_switch_state(int32_t state)
{
	/* change state */
	smartlamp_switch_state = state; 
	if (state == SMARTLAMP_SWITCH_ON) {
		gpio_set_level(GPIO_OUTPUT_NOTIFICATION_LED, NOTIFICATION_LED_GPIO_ON);
		gpio_set_level(GPIO_OUTPUT_MAINLED, MAINLED_GPIO_ON);
	} else {
		gpio_set_level(GPIO_OUTPUT_NOTIFICATION_LED, NOTIFICATION_LED_GPIO_OFF);
		gpio_set_level(GPIO_OUTPUT_MAINLED, MAINLED_GPIO_OFF);
	}
}

static void send_switch_cap_evt(IOT_CAP_HANDLE *handle, int32_t state)
{
	IOT_EVENT *switch_evt;
	uint8_t evt_num = 1;
	int32_t sequence_no;

	/* Setup switch onoff state */
	if (state == SMARTLAMP_SWITCH_ON) {
		switch_evt = st_cap_attr_create_string("switch", "on", NULL);
	} else {
		switch_evt = st_cap_attr_create_string("switch", "off", NULL);
	}

	/* Send switch onoff event */
	sequence_no = st_cap_attr_send(handle, evt_num, &switch_evt);
	if (sequence_no < 0) {
		SL_LOGE(TAG, "fail to send switch onoff data");
	}

	SL_LOGI(TAG, "Sequence number return : %d", sequence_no);
	st_cap_attr_free(switch_evt);
}

static void button_event(IOT_CAP_HANDLE *handle, uint32_t type, uint32_t count)
{
	if (!handle) {
		return;
	}

	if (type == BUTTON_SHORT_PRESS) {
		SL_LOGI(TAG, "Button short press, count: %d\n", count);
		switch (count) {
		case 1:
			if (g_iot_status == IOT_STATUS_NEED_INTERACT) {
				st_conn_ownership_confirm(ctx, true);
				gpio_set_level(GPIO_OUTPUT_NOTIFICATION_LED, NOTIFICATION_LED_GPIO_OFF);
				noti_led_mode = LED_ANIMATION_MODE_IDLE;
			} else {
				/* change switch state and LED state */
				if (smartlamp_switch_state == SMARTLAMP_SWITCH_ON) {
					change_switch_state(SMARTLAMP_SWITCH_OFF);
					send_switch_cap_evt(handle, SMARTLAMP_SWITCH_OFF);
				} else {
					change_switch_state(SMARTLAMP_SWITCH_ON);
					send_switch_cap_evt(handle, SMARTLAMP_SWITCH_ON);
				}
			}
			break;

		default:
			led_blink(GPIO_OUTPUT_NOTIFICATION_LED, 100, count);
			break;
		}
	} else if (type == BUTTON_LONG_PRESS) {
		SL_LOGI(TAG, "Button long press, count: %d\n", count);
		led_blink(GPIO_OUTPUT_NOTIFICATION_LED, 100, 3);
		/* clean-up provisioning & registered data with reboot option*/
		st_conn_cleanup(ctx, true);
	}
}

static void iot_status_cb(iot_status_t status, iot_stat_lv_t stat_lv, void *usr_data)
{
	g_iot_status = status;
	SL_LOGI(TAG, "iot_status: %d, lv: %d\n", status, stat_lv);

	switch (status) {
	case IOT_STATUS_NEED_INTERACT:
		noti_led_mode = LED_ANIMATION_MODE_FAST;
		break;
	case IOT_STATUS_IDLE:
	case IOT_STATUS_CONNECTING:
		noti_led_mode = LED_ANIMATION_MODE_IDLE;
		if (smartlamp_switch_state == SMARTLAMP_SWITCH_ON) {
			gpio_set_level(GPIO_OUTPUT_NOTIFICATION_LED, NOTIFICATION_LED_GPIO_ON);
		} else {
			gpio_set_level(GPIO_OUTPUT_NOTIFICATION_LED, NOTIFICATION_LED_GPIO_OFF);
		}
		break;
	default:
		break;
	}
}

void cap_switch_init_cb(IOT_CAP_HANDLE *handle, void *usr_data)
{
	IOT_EVENT *init_evt;
	uint8_t evt_num = 1;
	int32_t sequence_no;

	/* Setup switch on state */
	init_evt = st_cap_attr_create_string("switch", "on", NULL);

	/* Send switch on event */
	sequence_no = st_cap_attr_send(handle, evt_num, &init_evt);
	if (sequence_no < 0) {
		SL_LOGE(TAG, "fail to send init_data");
	}

	SL_LOGI(TAG, "Sequence number return : %d", sequence_no);
	st_cap_attr_free(init_evt);
}



void cap_switch_cmd_off_cb(IOT_CAP_HANDLE *handle, iot_cap_cmd_data_t *cmd_data, void *usr_data)
{
	IOT_EVENT *off_evt;
	uint8_t evt_num = 1;
	int32_t sequence_no;

	SL_LOGI(TAG, "called [%s] func with : num_args:%u",
		__func__, cmd_data->num_args);

	change_switch_state(SMARTLAMP_SWITCH_OFF);

	/* Setup switch off state */
	off_evt = st_cap_attr_create_string("switch", "off", NULL);

	/* Send switch off event */
	sequence_no = st_cap_attr_send(handle, evt_num, &off_evt);
	if (sequence_no < 0) {
		SL_LOGE(TAG, "fail to send off_data");
	}

	SL_LOGI(TAG, "Sequence number return : %d", sequence_no);
	st_cap_attr_free(off_evt);
}


void cap_switch_cmd_on_cb(IOT_CAP_HANDLE *handle, iot_cap_cmd_data_t *cmd_data, void *usr_data)
{
	IOT_EVENT *on_evt;
	uint8_t evt_num = 1;
	int32_t sequence_no;

	SL_LOGI(TAG, "called [%s] func with : num_args:%u",
		__func__, cmd_data->num_args);

	change_switch_state(SMARTLAMP_SWITCH_ON);

	/* Setup switch on state */
	on_evt = st_cap_attr_create_string("switch", "on", NULL);

	/* Send switch on event */
	sequence_no = st_cap_attr_send(handle, evt_num, &on_evt);
	if (sequence_no < 0) {
		SL_LOGE(TAG, "fail to send on_data");
	}

	SL_LOGI(TAG, "Sequence number return : %d", sequence_no);
	st_cap_attr_free(on_evt);
}

void cap_switch_noti_cb(iot_noti_data_t *noti_data, void *noti_usr_data)
{
	SL_LOGI(TAG, "Notification message received");

	if (noti_data->type == IOT_NOTI_TYPE_DEV_DELETED) {
		SL_LOGI(TAG, "[device deleted]");
	} else if (noti_data->type == IOT_NOTI_TYPE_RATE_LIMIT) {
		SL_LOGI(TAG, "[rate limit] Remaining time:%d, sequence number:%d",
			noti_data->raw.rate_limit.remainingTime, noti_data->raw.rate_limit.sequenceNumber);
	}
}

static void smartswitch_task(void *arg)
{
	IOT_CAP_HANDLE *handle = (IOT_CAP_HANDLE *)arg;

	int button_event_type;
	int button_event_count;

	for (;;) {
		if (get_button_event(&button_event_type, &button_event_count)) {
			button_event(handle, button_event_type, button_event_count);
		}

		if (noti_led_mode != LED_ANIMATION_MODE_IDLE) {
			change_led_state(noti_led_mode);
		}
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
void smart_switch_main(void)
#endif
{
	/**
	  SmartThings Device Kit(STDK) aims to make it easier to develop IoT devices by providing
	  additional st_iot_core layer to the existing chip vendor SW Architecture.

	  That is, you can simply develop a basic application by just calling the APIs provided by st_iot_core layer
	  like below. st_iot_core currently offers 14 API.

	  //create a iot context
	  1. st_conn_init();

	  //create a handle to process capability
	  2. st_cap_handle_init();

	  //register a callback function to process capability command when it comes from the SmartThings Server.
	  3. st_cap_cmd_set_cb();

	  //needed when it is necessary to keep monitoring the device status
	  4. user_defined_task()

	  //process on-boarding procedure. There is nothing more to do on the app side than call the API.
	  5. st_conn_start();
	 */

	IOT_CAP_HANDLE* switch_handle = NULL;
	int iot_err;

	// 1. create a iot context
	ctx = st_conn_init(onboarding_config, onboarding_config_len, device_info, device_info_len);
	if (ctx != NULL) {
		iot_err = st_conn_set_noti_cb(ctx, cap_switch_noti_cb, NULL);
		if (iot_err)
			SL_LOGE(TAG, "fail to set notification callback function");

	// 2. create a handle to process capability
	//	implement init_callback function
		switch_handle = st_cap_handle_init(ctx, "main", "switch", cap_switch_init_cb, NULL);

	// 3. register a callback function to process capability command when it comes from the SmartThings Server
	//	implement callback function
		iot_err = st_cap_cmd_set_cb(switch_handle, "off", cap_switch_cmd_off_cb, NULL);
		if (iot_err)
			SL_LOGE(TAG, "fail to set cmd_cb for off");
		iot_err = st_cap_cmd_set_cb(switch_handle, "on", cap_switch_cmd_on_cb, NULL);
		if (iot_err)
			SL_LOGE(TAG, "fail to set cmd_cb for on");

	} else {
		SL_LOGE(TAG, "fail to create the iot_context");
		return;
	}

	gpio_init();

	// 4. needed when it is necessary to keep monitoring the device status
	iot_os_thread_create(smartswitch_task, "smartswitch_task", 2048, (void *)switch_handle, 100, NULL);

	// 5. process on-boarding procedure. There is nothing more to do on the app side than call the API.
	st_conn_start(ctx, (st_status_cb)&iot_status_cb, IOT_STATUS_ALL, NULL, NULL);

}
