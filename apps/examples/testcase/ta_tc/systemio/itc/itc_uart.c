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

/// @file itc_uart.c

/// @brief Test Case Iotbus APIs for UART

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>
#include <iotbus_uart.h>
#include "itc_internal.h"
#include "iotbus_error.h"
#include <string.h>

#define MICROSECOND 1000000
#define BUF_LEN 32
#ifdef CONFIG_ARCH_CHIP_STM32
#define DEVPATH "/dev/ttyS1"
#else                            // artik
#define DEVPATH "/dev/ttyS2"
#endif

/**
* @testcase         itc_systemio_iotbus_uart_init_stop_p
* @brief            initializes and closes uart_context
* @scenario         initializes and closes uart_context
* @apicovered       iotbus_uart_init, iotbus_uart_stop
* @precondition     none
* @postcondition    none
*/
void itc_systemio_iotbus_uart_init_stop_p(void)
{
	int ret = IOTBUS_ERROR_NONE;
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_init_stop_p_multi_handle
* @brief            initializes and closes uart_context
* @scenario         initializes and closes uart_context
* @apicovered       iotbus_uart_init, iotbus_uart_stop
* @precondition     none
* @postcondition    none
*/
static void itc_systemio_iotbus_uart_init_stop_p_multi_handle(void)
{
	int ret = IOTBUS_ERROR_NONE;

	iotbus_uart_context_h h_uart1 = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init1", h_uart1, NULL);

	iotbus_uart_context_h h_uart2 = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ_CLEANUP("iotbus_uart_init2", h_uart2, NULL, iotbus_uart_stop(h_uart1));

	iotbus_uart_context_h h_uart3 = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ_CLEANUP("iotbus_uart_init3", h_uart3, NULL, iotbus_uart_stop(h_uart2); iotbus_uart_stop(h_uart1));

	ret = iotbus_uart_stop(h_uart3);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_stop3", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart2); iotbus_uart_stop(h_uart1));

	ret = iotbus_uart_stop(h_uart2);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_stop2", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart1));

	ret = iotbus_uart_stop(h_uart1);
	TC_ASSERT_EQ("iotbus_uart_stop1", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_set_baudrate_p
* @brief            sets uart baud rate
* @scenario         sets uart baud rate
* @apicovered       iotbus_uart_set_baudrate
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
void itc_systemio_iotbus_uart_set_baudrate_p(void)
{
	int i_baudrate = 115200;
	int ret = IOTBUS_ERROR_NONE;
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_set_baudrate(h_uart, i_baudrate);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_baudrate", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();;
}

/**
* @testcase         itc_systemio_iotbus_uart_set_baudrate_n
* @brief            sets uart baud rate
* @scenario         sets uart baud rate
* @apicovered       iotbus_uart_set_baudrate
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_set_baudrate_n(void)
{
	int i_baudrate = -115200; // invalid baudrate
	int ret = IOTBUS_ERROR_NONE;

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_set_baudrate(h_uart, i_baudrate);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_baudrate", ret, IOTBUS_ERROR_INVALID_PARAMETER, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_set_mode_p
* @brief            sets byte size, parity bit and stop bits
* @scenario         sets byte size, parity bit and stop bits
* @apicovered       iotbus_uart_set_mode
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
void itc_systemio_iotbus_uart_set_mode_p(void)
{
	int i_bytesize = 8;
	int i_stop_bits = 1;
	int ret = IOTBUS_ERROR_NONE;
	int mode[] = { IOTBUS_UART_PARITY_NONE, IOTBUS_UART_PARITY_EVEN, IOTBUS_UART_PARITY_ODD };
	int i_modes = sizeof(mode) / sizeof(int);
	int index = 0;
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	for (index = 0; index < i_modes; index++) {
		ret = iotbus_uart_set_mode(h_uart, i_bytesize, mode[index], i_stop_bits);
		TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_mode", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));
	}

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_set_mode_p_all
* @brief            sets byte size, parity bit and stop bits (all combinations)
* @scenario         sets byte size, parity bit and stop bits (all combinations)
* @apicovered       iotbus_uart_set_mode
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_set_mode_p_all(void)
{
	int i_bytesize = 0;
	int i_max_bytesize = 10;
	int i_stop_bits = 0;
	int i_max_stop_bits = 4;
	int ret = IOTBUS_ERROR_NONE;
	int mode[] = { IOTBUS_UART_PARITY_NONE, IOTBUS_UART_PARITY_EVEN, IOTBUS_UART_PARITY_ODD };
	char mode_str[3][BUF_LEN] = { "IOTBUS_UART_PARITY_NONE", "IOTBUS_UART_PARITY_EVEN", "IOTBUS_UART_PARITY_ODD" };
	int i_modes = sizeof(mode) / sizeof(int);
	int index = 0;
	int count_bytesize_fail = 0;
	int count_stopbit_fail = 0;
	int count_fail = 0;

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	for (i_bytesize = 0; i_bytesize < i_max_bytesize; ++i_bytesize) {
		for (i_stop_bits = 0; i_stop_bits < i_max_stop_bits; ++i_stop_bits) {
			for (index = 0; index < i_modes; index++) {
				ret = iotbus_uart_set_mode(h_uart, i_bytesize, mode[index], i_stop_bits);
				if (i_stop_bits < 1 || i_stop_bits > 2) {
					//checking for invalid parameters (stopbits)
					if (ret != IOTBUS_ERROR_INVALID_PARAMETER) {
						SYSIO_ITC_PRINT("iotbus_uart_set_mode failed for stopbits ::%d\n", i_stop_bits);
						count_stopbit_fail++;
					}
				} else if (i_bytesize < 5 || i_bytesize > 8) {
					//checking with invalid parameters (bytessize)
					if (ret != IOTBUS_ERROR_INVALID_PARAMETER) {
						SYSIO_ITC_PRINT("iotbus_uart_set_mode failed for bytesize ::%d\n", i_bytesize);
						count_bytesize_fail++;
					}
				} else {
					//checking with valid parameters
					if (ret != IOTBUS_ERROR_NONE) {
						SYSIO_ITC_PRINT("iotbus_uart_set_mode failed UART_PARITY::%s, bytesize::%d, stopbits::%d \n", mode_str[index], i_bytesize, i_stop_bits);
						count_fail++;
					}
				}
			}
		}
	}
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_mode fail_count with bytesize", count_bytesize_fail, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_mode fail_count with stopbit", count_stopbit_fail, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_mode fail_count", count_fail, 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_set_flowcontrol_p
* @brief            set flow control settings
* @scenario         set flow control settings
* @apicovered       iotbus_uart_set_flowcontrol
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
void itc_systemio_iotbus_uart_set_flowcontrol_p(void)
{
	int i_size = 4;
	int ret = IOTBUS_ERROR_NONE;
	int rtscts[4][2] = { {1, 0}, {0, 1}, {1, 1}, {0, 0} };
	int index = 0;
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	for (index = 0; index < i_size; index++) {
		ret = iotbus_uart_set_flowcontrol(h_uart, rtscts[index][0], rtscts[index][1]);
		TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_flowcontrol", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));
	}

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_write_read_p
* @brief            write and read data over uart bus
* @scenario         write and read data over uart bus
* @apicovered       iotbus_uart_write, iotbus_uart_read
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
void itc_systemio_iotbus_uart_write_read_p(void)
{
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_output_text[BUF_LEN];
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_write(h_uart, sz_input_text, sizeof(sz_input_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write", ret < 0, false, iotbus_uart_stop(h_uart));

	usleep(MICROSECOND);

	ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", ret < 0, false, iotbus_uart_stop(h_uart));

	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", strcmp(sz_input_text, sz_output_text), 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_flush_p
* @brief            flushes uart buffer
* @scenario         flushes uart buffer
* @apicovered       iotbus_uart_flush
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
void itc_systemio_iotbus_uart_flush_p(void)
{
	int ret = IOTBUS_ERROR_NONE;
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_flush(h_uart);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_flush", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_write_read_flush_p
* @brief            write, read, flush data over uart bus
* @scenario         write, read, flush data over uart bus
* @apicovered       iotbus_uart_write, iotbus_uart_read, iotbus_uart_flush
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_write_read_flush_p(void)
{
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_output_text[BUF_LEN];

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_write(h_uart, sz_input_text, sizeof(sz_input_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write", ret < 0, false, iotbus_uart_stop(h_uart));

	usleep(MICROSECOND);

	ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", ret < 0, false, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", strcmp(sz_input_text, sz_output_text), 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_flush(h_uart);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_flush", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_write_read_flush_multi_p_multi_handle
* @brief            write, read, flush data over uart bus with multi handler
* @scenario         write, read, flush data over uart bus with multi handler
* @apicovered       iotbus_uart_write, iotbus_uart_read, iotbus_uart_flush
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_write_read_flush_p_multi_handle(void)
{
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text1[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_input_text2[BUF_LEN] = "UART READ/WRITE ITC TESTING1!";
	char sz_output_text[BUF_LEN];

	iotbus_uart_context_h h_uart1 = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart1, NULL);

	iotbus_uart_context_h h_uart2 = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ_CLEANUP("iotbus_uart_init", h_uart2, NULL, iotbus_uart_stop(h_uart1));

	ret = iotbus_uart_write(h_uart1, sz_input_text1, sizeof(sz_input_text1));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write", ret < 0, false, iotbus_uart_stop(h_uart1); iotbus_uart_stop(h_uart2));

	ret = iotbus_uart_write(h_uart2, sz_input_text2, sizeof(sz_input_text2));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write", ret < 0, false, iotbus_uart_stop(h_uart1); iotbus_uart_stop(h_uart2));

	usleep(MICROSECOND);

	ret = iotbus_uart_read(h_uart1, sz_output_text, sizeof(sz_output_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", ret < 0, false, iotbus_uart_stop(h_uart1); iotbus_uart_stop(h_uart2));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", strcmp(sz_input_text1, sz_output_text), 0, iotbus_uart_stop(h_uart1); iotbus_uart_stop(h_uart2));

	ret = iotbus_uart_read(h_uart2, sz_output_text, sizeof(sz_output_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", ret < 0, false, iotbus_uart_stop(h_uart1); iotbus_uart_stop(h_uart2));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", strcmp(sz_input_text2, sz_output_text), 0, iotbus_uart_stop(h_uart1); iotbus_uart_stop(h_uart2));

	ret = iotbus_uart_flush(h_uart1);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_flush", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart1); iotbus_uart_stop(h_uart2));

	ret = iotbus_uart_flush(h_uart2);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_flush", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart1); iotbus_uart_stop(h_uart2));

	ret = iotbus_uart_stop(h_uart2);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart1));

	ret = iotbus_uart_stop(h_uart1);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

#if 0
/**
* @testcase         itc_systemio_iotbus_uart_write_read_flush_read_p
* @brief            perform write, read, flush, and read data operations over uart bus
* @scenario         perform write, read, flush, and read data operations over uart bus
* @apicovered       iotbus_uart_write, iotbus_uart_read, iotbus_uart_flush
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_write_read_flush_read_p(void)
{
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_output_text[BUF_LEN];

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_write(h_uart, sz_input_text, sizeof(sz_input_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write", ret < 0, false, iotbus_uart_stop(h_uart));

	usleep(MICROSECOND);

	ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", ret < 0, false, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", strcmp(sz_input_text, sz_output_text), 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_flush(h_uart);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_flush", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read", ret == 0, true, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}
#endif

/**
* @testcase         itc_systemio_iotbus_uart_set_flow_write_read_p
* @brief            write and read data over uart bus with set flow
* @scenario         write and read data over uart bus with set flow
* @apicovered       iotbus_uart_write, iotbus_uart_read, iotbus_uart_set_flowcontrol
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_set_flow_write_read_p(void)
{
	int i_size = 4;
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_output_text[BUF_LEN];
	int rtscts[4][2] = { {1, 0}, {0, 1}, {1, 1}, {0, 0} };
	int index = 0;
	int count_fail_flowcontrol = 0;
	int count_fail_write = 0;
	int count_fail_read = 0;

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	for (index = 0; index < i_size; index++) {
		ret = iotbus_uart_set_flowcontrol(h_uart, rtscts[index][0], rtscts[index][1]);
		if (ret != IOTBUS_ERROR_NONE) {
			SYSIO_ITC_PRINT("iotbus_uart_set_flowcontrol failed with xonxoff ::%d, rtscts::%d\n", rtscts[index][0], rtscts[index][1]);
			count_fail_flowcontrol++;
			continue;
		}

		ret = iotbus_uart_write(h_uart, sz_input_text, sizeof(sz_input_text));
		if (ret < 0) {
			SYSIO_ITC_PRINT("iotbus_uart_write failed with xonxoff ::%d, rtscts::%d\n", rtscts[index][0], rtscts[index][1]);
			count_fail_write++;
			continue;
		}

		usleep(MICROSECOND);

		ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
		if ((ret < 0) || (strcmp(sz_input_text, sz_output_text) != 0)) {
			SYSIO_ITC_PRINT("iotbus_uart_read failed with xonxoff ::%d, rtscts::%d\n", rtscts[index][0], rtscts[index][1]);
			count_fail_read++;
		}
	}
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write failed count", count_fail_write, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read failed count", count_fail_read, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_flowcontrol failed count", count_fail_flowcontrol, 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_set_flow_write_flush_read_p
* @brief            flush, write, and read data over uart bus with set flow
* @scenario         flush, write, and read data over uart bus with set flow
* @apicovered       iotbus_uart_write, iotbus_uart_read, iotbus_uart_set_flowcontrol, iotbus_uart_flush
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_set_flow_flush_write_read_p(void)
{
	int i_size = 4;
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_output_text[BUF_LEN];
	int rtscts[4][2] = { {1, 0}, {0, 1}, {1, 1}, {0, 0} };
	int index = 0;
	int count_fail_flowcontrol = 0;
	int count_fail_write = 0;
	int count_fail_read = 0;
	int count_fail_flush = 0;

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	for (index = 0; index < i_size; index++) {
		ret = iotbus_uart_set_flowcontrol(h_uart, rtscts[index][0], rtscts[index][1]);
		if (ret != IOTBUS_ERROR_NONE) {
			SYSIO_ITC_PRINT("iotbus_uart_set_flowcontrol failed with xonxoff ::%d, rtscts::%d\n", rtscts[index][0], rtscts[index][1]);
			count_fail_flowcontrol++;
			continue;
		}

		ret = iotbus_uart_flush(h_uart);
		if (ret != IOTBUS_ERROR_NONE) {
			SYSIO_ITC_PRINT("iotbus_uart_flush failed with xonxoff ::%d, rtscts::%d\n", rtscts[index][0], rtscts[index][1]);
			count_fail_flush++;
			continue;
		}

		ret = iotbus_uart_write(h_uart, sz_input_text, sizeof(sz_input_text));
		if (ret < 0) {
			SYSIO_ITC_PRINT("iotbus_uart_write failed with xonxoff ::%d, rtscts::%d\n", rtscts[index][0], rtscts[index][1]);
			count_fail_write++;
			continue;
		}

		usleep(MICROSECOND);

		ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
		if ((ret < 0) || (strcmp(sz_input_text, sz_output_text) != 0)) {
			SYSIO_ITC_PRINT("iotbus_uart_read failed with xonxoff ::%d, rtscts::%d\n", rtscts[index][0], rtscts[index][1]);
			count_fail_read++;
		}
	}

	TC_ASSERT_EQ_CLEANUP("iotbus_uart_flush failed count", count_fail_flush, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write failed count", count_fail_write, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read failed count", count_fail_read, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_flowcontrol failed count", count_fail_flowcontrol, 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_set_mode_write_read_p
* @brief            To check write/read operation with different parity
* @scenario         uart write/read operation with different parity
* @apicovered       iotbus_uart_set_mode, iotbus_uart_write, iotbus_uart_read
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_set_mode_write_read_p(void)
{
	int i_bytesize = 8;
	int i_stop_bits = 1;
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_output_text[BUF_LEN];
	int mode[] = { IOTBUS_UART_PARITY_NONE, IOTBUS_UART_PARITY_EVEN, IOTBUS_UART_PARITY_ODD };
	char *mode_str[3] = { "IOTBUS_UART_PARITY_NONE", "IOTBUS_UART_PARITY_EVEN", "IOTBUS_UART_PARITY_ODD" };
	int i_modes = sizeof(mode) / sizeof(int);
	int index = 0;
	int count_fail_mode = 0;
	int count_fail_write = 0;
	int count_fail_read = 0;

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	for (index = 0; index < i_modes; index++) {
		ret = iotbus_uart_set_mode(h_uart, i_bytesize, mode[index], i_stop_bits);
		if ((ret != IOTBUS_ERROR_NONE)) {
			SYSIO_ITC_PRINT("iotbus_uart_set_mode failed with PARITY ::%s, stop_bits::%d\n", mode_str[index], i_stop_bits);
			count_fail_mode++;
			continue;
		}

		ret = iotbus_uart_write(h_uart, sz_input_text, sizeof(sz_input_text));
		if ((ret < 0)) {
			SYSIO_ITC_PRINT("iotbus_uart_write failed with PARITY ::%s, stop_bits::%d\n", mode_str[index], i_stop_bits);
			count_fail_write++;
			continue;
		}

		usleep(MICROSECOND);

		ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
		if ((ret < 0) || (strcmp(sz_input_text, sz_output_text) != 0)) {
			SYSIO_ITC_PRINT("iotbus_uart_read failed with PARITY ::%s, stop_bits::%d\n", mode_str[index], i_stop_bits);
			count_fail_read++;
		}
	}

	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_mode failed count", count_fail_mode, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write failed count", count_fail_write, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read failed count", count_fail_read, 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_set_mode_flush_write_read_p
* @brief            To check flush/write/read operation with different parity
* @scenario         uart flush/write/read operation with different parity
* @apicovered       iotbus_uart_set_mode, iotbus_uart_write, iotbus_uart_read, iotbus_uart_flush
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_set_mode_flush_write_read_p(void)
{
	int i_bytesize = 8;
	int i_stop_bits = 1;
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_output_text[BUF_LEN];
	int mode[] = { IOTBUS_UART_PARITY_NONE, IOTBUS_UART_PARITY_EVEN, IOTBUS_UART_PARITY_ODD };
	char *mode_str[3] = { "IOTBUS_UART_PARITY_NONE", "IOTBUS_UART_PARITY_EVEN", "IOTBUS_UART_PARITY_ODD" };
	int i_modes = sizeof(mode) / sizeof(int);
	int index = 0;
	int count_fail_mode = 0;
	int count_fail_write = 0;
	int count_fail_read = 0;
	int count_fail_flush = 0;

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	for (index = 0; index < i_modes; index++) {
		ret = iotbus_uart_set_mode(h_uart, i_bytesize, mode[index], i_stop_bits);
		if ((ret != IOTBUS_ERROR_NONE)) {
			SYSIO_ITC_PRINT("iotbus_uart_set_mode failed with PARITY ::%s, stop_bits::%d\n", mode_str[index], i_stop_bits);
			count_fail_mode++;
			continue;
		}

		ret = iotbus_uart_flush(h_uart);
		if ((ret != IOTBUS_ERROR_NONE)) {
			SYSIO_ITC_PRINT("iotbus_uart_flush failed with PARITY ::%s, stop_bits::%d\n", mode_str[index], i_stop_bits);
			count_fail_flush++;
			continue;
		}

		ret = iotbus_uart_write(h_uart, sz_input_text, sizeof(sz_input_text));
		if ((ret < 0)) {
			SYSIO_ITC_PRINT("iotbus_uart_write failed with PARITY ::%s, stop_bits::%d\n", mode_str[index], i_stop_bits);
			count_fail_write++;
			continue;
		}

		usleep(MICROSECOND);

		ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
		if ((ret < 0) || (strcmp(sz_input_text, sz_output_text) != 0)) {
			SYSIO_ITC_PRINT("iotbus_uart_read failed with PARITY ::%s, stop_bits::%d\n", mode_str[index], i_stop_bits);
			count_fail_read++;
		}
	}

	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_mode failed count", count_fail_mode, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_flush failed count", count_fail_flush, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write failed count", count_fail_write, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read failed count", count_fail_read, 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_set_flow_mode_flush_write_read_p
* @brief            To check flush/write/read operation with all combination of parity and flow
* @scenario         uart flush/write/read operation with all combination of parity and flow
* @apicovered       iotbus_uart_set_flowcontrol, iotbus_uart_set_mode, iotbus_uart_write, iotbus_uart_read, iotbus_uart_flush
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_set_flow_mode_flush_write_read_p(void)
{
	int i_bytesize = 8;
	int i_flow_size = 4;
	int i_stop_bits = 1;
	int ret = IOTBUS_ERROR_NONE;
	char sz_input_text[BUF_LEN] = "UART READ/WRITE ITC TESTING!";
	char sz_output_text[BUF_LEN];
	int mode[] = { IOTBUS_UART_PARITY_NONE, IOTBUS_UART_PARITY_EVEN, IOTBUS_UART_PARITY_ODD };
	char *mode_str[3] = { "IOTBUS_UART_PARITY_NONE", "IOTBUS_UART_PARITY_EVEN", "IOTBUS_UART_PARITY_ODD" };
	int rtscts[4][2] = { {1, 0}, {0, 1}, {1, 1}, {0, 0} };
	int i_modes = sizeof(mode) / sizeof(int);
	int index = 0;
	int flow_index = 0;
	int count_fail_mode = 0;
	int count_fail_write = 0;
	int count_fail_read = 0;
	int count_fail_flush = 0;
	int count_fail_flowcontrol = 0;

	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	for (flow_index = 0; flow_index < i_flow_size; flow_index++) {
		ret = iotbus_uart_set_flowcontrol(h_uart, rtscts[flow_index][0], rtscts[flow_index][1]);
		if (ret != IOTBUS_ERROR_NONE) {
			SYSIO_ITC_PRINT("iotbus_uart_set_flowcontrol failed with xonxoff ::%d, rtscts::%d\n", rtscts[flow_index][0], rtscts[flow_index][1]);
			count_fail_flowcontrol++;
			continue;
		}

		for (index = 0; index < i_modes; index++) {
			ret = iotbus_uart_set_mode(h_uart, i_bytesize, mode[index], i_stop_bits);
			if ((ret != IOTBUS_ERROR_NONE)) {
				SYSIO_ITC_PRINT("iotbus_uart_set_mode failed with PARITY ::%s, stop_bits::%d, xonxoff::%d, rtscts::%d\n", mode_str[index], i_stop_bits, rtscts[flow_index][0], rtscts[flow_index][1]);
				count_fail_mode++;
				continue;
			}

			ret = iotbus_uart_flush(h_uart);
			if ((ret != IOTBUS_ERROR_NONE)) {
				SYSIO_ITC_PRINT("iotbus_uart_flush failed with PARITY ::%s, stop_bits::%d, xonxoff::%d, rtscts::%d\n", mode_str[index], i_stop_bits, rtscts[flow_index][0], rtscts[flow_index][1]);
				count_fail_flush++;
				continue;
			};

			ret = iotbus_uart_write(h_uart, sz_input_text, sizeof(sz_input_text));
			if ((ret < 0)) {
				SYSIO_ITC_PRINT("iotbus_uart_write failed with PARITY ::%s, stop_bits::%d, xonxoff::%d, rtscts::%d\n", mode_str[index], i_stop_bits, rtscts[flow_index][0], rtscts[flow_index][1]);
				count_fail_write++;
				continue;
			}

			usleep(MICROSECOND);

			ret = iotbus_uart_read(h_uart, sz_output_text, sizeof(sz_output_text));
			if ((ret < 0) || (strcmp(sz_input_text, sz_output_text) != 0)) {
				SYSIO_ITC_PRINT("iotbus_uart_read failed with PARITY ::%s, stop_bits::%d, xonxoff::%d, rtscts::%d\n", mode_str[index], i_stop_bits, rtscts[flow_index][0], rtscts[flow_index][1]);
				count_fail_read++;
			}
		}
	}
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_flowcontrol failed count", count_fail_flowcontrol, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_set_mode failed count", count_fail_mode, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_flush failed count", count_fail_flush, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_write failed count", count_fail_write, 0, iotbus_uart_stop(h_uart));
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_read failed count", count_fail_read, 0, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_rxavailable
* @brief            To check if there is data in rx fifo.
* @scenario         if data in rx fifo
* @apicovered       iotbus_uart_rxavailable
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_rxavailable(void)
{
	int ret = IOTBUS_ERROR_NONE;
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_rxavailable(h_uart);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_rxavailable", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_txready
* @brief            To check if the transmit fifo is not full
* @scenario         if transmit fifo is not full.
* @apicovered       iotbus_uart_txready
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_txready(void)
{
	int ret = IOTBUS_ERROR_NONE;
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_txready(h_uart);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_txready", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/**
* @testcase         itc_systemio_iotbus_uart_txempty
* @brief            To check if the transmit fifo is empty
* @scenario         if transmit fifo is empty.
* @apicovered       iotbus_uart_txempty
* @precondition     initializes uart_context
* @postcondition    closes uart_context
*/
static void itc_systemio_iotbus_uart_txempty(void)
{
	int ret = IOTBUS_ERROR_NONE;
	iotbus_uart_context_h h_uart = iotbus_uart_init(DEVPATH);
	TC_ASSERT_NEQ("iotbus_uart_init", h_uart, NULL);

	ret = iotbus_uart_txempty(h_uart);
	TC_ASSERT_EQ_CLEANUP("iotbus_uart_txempty", ret, IOTBUS_ERROR_NONE, iotbus_uart_stop(h_uart));

	ret = iotbus_uart_stop(h_uart);
	TC_ASSERT_EQ("iotbus_uart_stop", ret, IOTBUS_ERROR_NONE);

	TC_SUCCESS_RESULT();
}

/****************************************************************************
 * Name: environ
 ****************************************************************************/

int itc_uart_main(void)
{
#ifndef CONFIG_SERIAL_TERMIOS
	SYSIO_ITC_UART_PRINT("IOTBUS Not Supported\n");
#else
	itc_systemio_iotbus_uart_init_stop_p();
	itc_systemio_iotbus_uart_init_stop_p_multi_handle();
	itc_systemio_iotbus_uart_set_baudrate_p();
	itc_systemio_iotbus_uart_set_baudrate_n();
	itc_systemio_iotbus_uart_set_mode_p();
	itc_systemio_iotbus_uart_set_mode_p_all();
	itc_systemio_iotbus_uart_set_flowcontrol_p();
	itc_systemio_iotbus_uart_write_read_p();
	itc_systemio_iotbus_uart_write_read_flush_p_multi_handle();
	itc_systemio_iotbus_uart_flush_p();
	itc_systemio_iotbus_uart_write_read_flush_p();
	//itc_systemio_iotbus_uart_write_read_flush_read_p();// TASH being hanged on execution of the testcase
	itc_systemio_iotbus_uart_set_flow_write_read_p();
	itc_systemio_iotbus_uart_set_flow_flush_write_read_p();
	itc_systemio_iotbus_uart_set_mode_write_read_p();
	itc_systemio_iotbus_uart_set_mode_flush_write_read_p();
	itc_systemio_iotbus_uart_set_flow_mode_flush_write_read_p();
	itc_systemio_iotbus_uart_rxavailable();
	itc_systemio_iotbus_uart_txready();
	itc_systemio_iotbus_uart_txempty();

#endif

	return 0;
}
