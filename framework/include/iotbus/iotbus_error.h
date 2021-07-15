/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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

/**
 * @defgroup IOTBUS IOTBUS
 * @ingroup IOTBUS
 * @{
 */

/**
 * @file iotbus/iotbus_error.h
 * @brief Definition of iotbus_error
 */

#ifndef IOTBUS_ERROR_H_
#define IOTBUS_ERROR_H_

#include <errno.h>

/**
 * @brief Enumeration for the iotbus error
 * @details
 * Enumeration Details:\n
 * IOTBUS_ERROR_NONE = 0, Successful\n
 * IOTBUS_ERROR_INVALID_PARAMETER = -EINVAL, Invalid parameter : -22\n
 * IOTBUS_ERROR_NO_DATA = -ENODATA, No data available : -61\n
 * IOTBUS_ERROR_TIMED_OUT = -ETIME, Time out : -62\n
 * IOTBUS_ERROR_NOT_IMPLEMENTED = -ENOSYS, Function not implemented : -38\n
 * IOTBUS_ERROR_NOT_SUPPORTED = -1101, Not supported\n
 * IOTBUS_ERROR_DEVICE_NOT_READY = -1102, Not ready to use\n
 * IOTBUS_ERROR_DEVICE_FAIL = -1103, Failed to use\n
 * IOTBUS_ERROR_UNKNOWN = -1999, Unknown error\n
 */
typedef enum {

	IOTBUS_ERROR_NONE = 0, /* Successful */

	IOTBUS_ERROR_INVALID_PARAMETER = -EINVAL, /* Invalid parameter : -22 */
	IOTBUS_ERROR_NO_DATA = -ENODATA, /* No data available : -61*/
	IOTBUS_ERROR_TIMED_OUT = -ETIME, /* Time out : -62 */
	IOTBUS_ERROR_NOT_IMPLEMENTED = -ENOSYS, /* Function not implemented : -38 */

	IOTBUS_ERROR_NOT_SUPPORTED = -1101, /* Not supported */
	IOTBUS_ERROR_DEVICE_NOT_READY = -1102, /* Device is Not ready to use */
	IOTBUS_ERROR_DEVICE_FAIL = -1103, /* Device is Not ready to use */
	IOTBUS_ERROR_QUEUE_FULL = -1104, /* Queue is full */
	IOTBUS_ERROR_OUT_OF_MEM = -1105, /* Heap is full */
	IOTBUS_ERROR_QUEUE_EMPTY = -1106, /* Queue is empty*/
	IOTBUS_ERROR_QUEUE_NOT_EMPTY = -1107, /*Queue is not empty*/

	IOTBUS_ERROR_UNKNOWN = -1999, /* Unknown error */
} iotbus_error_e;

#endif /* IOTBUS_ERROR_H_ */

/** @} */ // end of IOTBUS group
