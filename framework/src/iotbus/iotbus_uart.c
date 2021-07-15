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
 * @file iotbus_uart.c
 * @brief Iotbus APIs for UART
 */

#include <tinyara/config.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <tinyara/serial/tioctl.h>
#include <iotbus/iotbus_error.h>
#include <iotbus/iotbus_uart.h>

#ifndef CONFIG_IOTBUS_UART_EVENT_SIZE
#define CONFIG_IOTBUS_UART_EVENT_SIZE 3
#endif

struct _iotbus_uart_s {
	int fd;
	int device;
	uart_write_cb callback;
	int timeout;
	uint8_t *buf;
	size_t len;
	iotbus_uart_state_e rx_state;
	iotbus_uart_state_e tx_state;
};

struct _iotbus_uart_wrapper_s {
	struct _iotbus_uart_s *handle;
};

#ifdef CONFIG_SERIAL_TERMIOS
static int g_iotbus_uart_br[30] = {
B50, B75, B110, B134, B150,
B200, B300, B600, B1200, B1800,
B2400, B4800, B9600, B19200, B38400,
B57600, B115200, B128000, B230400, B256000,
B460800, B500000, B576000, B921600, B1000000,
B1152000, B1500000, B2000000, B2500000, B3000000, };

static int g_iotbus_uart_size = 30;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Private Functions
 */

#ifdef CONFIG_SERIAL_TERMIOS
static int _iotbus_valid_baudrate(unsigned int rate)
{
	int i = 0;
	for (; i < g_iotbus_uart_size; i++)
		if (rate == g_iotbus_uart_br[i])
			return 1;

	return 0;
}
#endif

static int _iotbus_get_dev_number(const char *path)
{
	
	const char *ptr = path;
	int len = strlen(path);
	char p[8] = { 0, };
	int cnt = 0;
	
	while (*ptr != 'S') {
		if (*ptr == 0) return -1;
		if (ptr - path >= len) return -1;
		ptr++;
	}

	for (ptr = ptr + 1; ptr < path + len; ptr++) {
		p[cnt++] = *ptr;
	}

	return atoi(p);
}

/*
 * Public Functions
 */
iotbus_uart_context_h iotbus_uart_init(const char *path)
{
	int fd;
	struct _iotbus_uart_s *handle;
	iotbus_uart_context_h dev;

	fd = open(path, O_RDWR, 0666);
	if (fd < 0) {
		return NULL;
	}

	handle = (struct _iotbus_uart_s *)malloc(sizeof(struct _iotbus_uart_s));
	if (!handle) {
		goto errout_with_close;
	}

	dev = (struct _iotbus_uart_wrapper_s *)malloc(sizeof(struct _iotbus_uart_wrapper_s));
	if (!dev) {
		free(handle);
		goto errout_with_close;
	}

	handle->fd = fd;
	handle->device = _iotbus_get_dev_number(path);
	handle->callback = NULL;
	dev->handle = handle;

	return dev;

errout_with_close:
	close(fd);
	return NULL;
}

iotbus_uart_context_h iotbus_uart_open(int device)
{
	iotbus_uart_context_h dev;
	char dev_path[16];

	if (device < 0) {
		return NULL;
	}

	snprintf(dev_path, sizeof(dev_path), "/dev/ttyS%d", device);
	dev = iotbus_uart_init(dev_path);

	return dev;
}

int iotbus_uart_stop(iotbus_uart_context_h hnd)
{
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;

	close(handle->fd);
	free(handle);
	hnd->handle = NULL;
	free(hnd);

	return IOTBUS_ERROR_NONE;
}

int iotbus_uart_rxavailable(iotbus_uart_context_h hnd)
{
	int ret;
	int fd;
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;
	fd = handle->fd;

	ret = tccheckfifo(fd, TIOCS_AVAIL);
	if (ret < 1) {
		return IOTBUS_ERROR_QUEUE_EMPTY;
	}

	return IOTBUS_ERROR_NONE;
}

int iotbus_uart_txready(iotbus_uart_context_h hnd)
{
	int ret;
	int fd;
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;
	fd = handle->fd;

	ret = tccheckfifo(fd, TIOCS_READY);
	if (ret < 1) {
		return IOTBUS_ERROR_QUEUE_FULL;
	}

	return IOTBUS_ERROR_NONE;
}

int iotbus_uart_txempty(iotbus_uart_context_h hnd)
{
	int ret;
	int fd;
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;
	fd = handle->fd;

	ret = tccheckfifo(fd, TIOCS_EMPTY);
	if (ret < 1) {
		return IOTBUS_ERROR_QUEUE_NOT_EMPTY;
	}

	return IOTBUS_ERROR_NONE;
}

#ifdef CONFIG_SERIAL_TERMIOS
int iotbus_uart_flush(iotbus_uart_context_h hnd)
{
	// tcdrain is not working even CONFIG_SERIAL_TERMIOS is defined.
	int fd;
	int ret;
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;
	fd = handle->fd;

	ret = tcflush(fd, TCIOFLUSH);
	if (ret < 0) {
		return IOTBUS_ERROR_NOT_SUPPORTED;
	}

	return IOTBUS_ERROR_NONE;
}

int iotbus_uart_set_baudrate(iotbus_uart_context_h hnd, unsigned int baud)
{
	int fd;
	int ret;
	struct termios tio;
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	if (!_iotbus_valid_baudrate(baud)) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;
	fd = handle->fd;

	ret = tcgetattr(fd, &tio);
	if (ret < 0) {
		return IOTBUS_ERROR_UNKNOWN;
	}
	tio.c_speed = baud;

	ret = tcsetattr(fd, TCSANOW, &tio);
	if (ret < 0) {
		return IOTBUS_ERROR_UNKNOWN;
	}

	return IOTBUS_ERROR_NONE;
}

// 8N1 would be set by calling "iotbus_uart_set_mode(hnd, 8,MRAA_UART_PARITY_NONE , 1)"
int iotbus_uart_set_mode(iotbus_uart_context_h hnd, int bytesize, iotbus_uart_parity_e parity, int stopbits)
{
	int fd;
	int ret;
	struct termios tio;
	struct _iotbus_uart_s *handle;
	int byteinfo[4] = { CS5, CS6, CS7, CS8 };

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;
	fd = handle->fd;

	ret = tcgetattr(fd, &tio);
	if (ret) {
		return IOTBUS_ERROR_UNKNOWN;
	}
	// set byte size
	if (bytesize < 5 || bytesize > 8) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= byteinfo[bytesize - 5];

	// set parity info
	switch (parity) {
	case IOTBUS_UART_PARITY_EVEN:
		tio.c_cflag |= PARENB;
		tio.c_cflag &= ~PARODD;
		break;
	case IOTBUS_UART_PARITY_ODD:
		tio.c_cflag |= PARENB;
		tio.c_cflag |= PARODD;
		break;
	case IOTBUS_UART_PARITY_NONE:
		tio.c_cflag &= ~PARENB;
		tio.c_cflag &= ~PARODD;
		break;
	default:
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	// set stop bit
	switch (stopbits) {
	case 1:
		tio.c_cflag &= ~CSTOPB;
		break;
	case 2:
		tio.c_cflag |= CSTOPB;
		break;
	default:
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	ret = tcsetattr(fd, TCSANOW, &tio);
	if (ret < 0) {
		return IOTBUS_ERROR_UNKNOWN;
	}

	return IOTBUS_ERROR_NONE;
}

// parameter
// rtscts => 1: rts/cts on, 0: off
int iotbus_uart_set_flowcontrol(iotbus_uart_context_h hnd, int xonxoff, int rtscts)
{
	int fd;
	int ret;
	struct termios tio;
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	if (xonxoff != 1 && xonxoff != 0) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	if (rtscts != 1 && rtscts != 0) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;

	fd = handle->fd;

	ret = tcgetattr(fd, &tio);
	if (ret < 0) {
		return IOTBUS_ERROR_UNKNOWN;
	}

	if (rtscts) {
		tio.c_cflag |= CRTS_IFLOW | CCTS_OFLOW;
	} else {
		tio.c_cflag &= ~(CRTS_IFLOW | CCTS_OFLOW);
	}

	if (xonxoff) {
		tio.c_iflag |= (IXON | IXOFF | IXANY);
	}

	ret = tcsetattr(fd, TCSANOW, &tio);
	if (ret < 0) {
		return IOTBUS_ERROR_UNKNOWN;
	}

	return IOTBUS_ERROR_NONE;
}
#endif

int iotbus_uart_read(iotbus_uart_context_h hnd, char *buf, unsigned int length)
{
	int fd;
	int ret;
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	if (!buf || length <= 0) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;

	fd = handle->fd;
	ret = read(fd, buf, length);
	if (ret < 0) {
		return IOTBUS_ERROR_UNKNOWN;
	}

	return ret;
}

int iotbus_uart_write(iotbus_uart_context_h hnd, const char *buf, unsigned int length)
{
	int fd;
	int ret;
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;

	if (!buf || length <= 0) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	fd = handle->fd;
	ret = write(fd, buf, length);
	if (ret < 0) {
		return IOTBUS_ERROR_UNKNOWN;
	}
	return ret;
}

#ifdef CONFIG_IOTDEV
/**
 * @brief Gets a device number of the UART.
 */
int iotbus_uart_get_device(iotbus_uart_context_h hnd)
{
	struct _iotbus_uart_s *handle;

	if (!hnd || !hnd->handle) {
		return IOTBUS_ERROR_INVALID_PARAMETER;
	}

	handle = (struct _iotbus_uart_s *)hnd->handle;
	return handle->device;
}
#endif

#ifdef __cplusplus
}
#endif
