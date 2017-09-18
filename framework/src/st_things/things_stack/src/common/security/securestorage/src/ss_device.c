/**************************************************************************** * * Copyright 2017 Samsung Electronics All Rights Reserved. * * Licensed under the Apache License, Version 2.0 (the "License"); * you may not use this file except in compliance with the License. * You may obtain a copy of the License at * * http://www.apache.org/licenses/LICENSE-2.0 * * Unless required by applicable law or agreed to in writing, * software distributed under the License is distributed on an * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, * either express or implied. See the License for the specific * language governing permissions and limitations under the License. * ****************************************************************************/#include <stdio.h>#include <stdlib.h>#include <string.h>#include <string.h>#include "ss_defines.h"#include "ss_device.h"#include "things_logger.h"#define TAG "SECURE_STORAGE"static get_device_unique_id g_get_device_unique_id = NULL;static char g_device_unique_id[SS_DEVICE_ID_MAX_SIZE];static unsigned int g_device_unique_id_len = 0;void ss_set_device_unique_id_cb(get_device_unique_id get_id_cb){	g_get_device_unique_id = get_id_cb;}/** * @brief    API to read device unique ID * * @param    p_id_buf            [out] Buffer for device unique ID * @param   p_id_buf_size      [in] size of p_id_buf * @param    p_id_len            [out] Length of ID * @return    SS_SUCCESS        if no error is occured */int ss_get_device_id(unsigned char *p_id_buf, size_t p_id_buf_size, unsigned int *p_id_len){	int ret = SS_NULL_REFERENCE;	if (NULL == p_id_buf || p_id_len == NULL || p_id_buf_size == 0)	{		THINGS_LOG_ERROR(THINGS_ERROR, TAG, "Invalid parameter..");		return SS_INVALID_ARGUMENT;	}	if (g_get_device_unique_id)	{		if (0 < g_device_unique_id_len)	{			if (p_id_buf_size >= g_device_unique_id_len) {				memcpy(p_id_buf, g_device_unique_id, g_device_unique_id_len);				*p_id_len = g_device_unique_id_len;				ret = SS_SUCCESS;			} else {				THINGS_LOG_ERROR(THINGS_ERROR, TAG, "DeviceID buffer too small..");				ret = SS_BUFFER_OVERRUN;			}		} else {			ret = g_get_device_unique_id(p_id_buf, p_id_buf_size, p_id_len);			if (SS_SUCCESS == ret) {				memset(g_device_unique_id, 0x00, SS_DEVICE_ID_MAX_SIZE);				memcpy(g_device_unique_id, p_id_buf, *p_id_len);				g_device_unique_id_len = *p_id_len;				THINGS_LOG(THINGS_INFO, TAG, "g_get_device_unique_id() was successfully done.");			}		}	} else {		THINGS_LOG_ERROR(THINGS_ERROR, TAG, "Can not find callback to read device unique ID.");		THINGS_LOG_ERROR(THINGS_ERROR, TAG, "Please set the callback using ssSetDeviceUniqueIDCallback() API.");		ret = SS_NULL_REFERENCE;	}	return ret;}