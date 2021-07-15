/****************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 *
 *   Copyright (C) 2020 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/irq.h>
#include <tinyara/arch.h>
#include <tinyara/serial/serial.h>
#include <tinyara/pm/pm.h>

#ifdef CONFIG_SERIAL_TERMIOS
#include <termios.h>
#endif

#include <arch/serial.h>
#include <arch/board/board.h>

#include "chip.h"
#include "up_arch.h"
#include "up_internal.h"

#include "mbed/hal/serial_api.h"
#include "mbed/targets/hal/rtl8721d/PinNames.h"
#include "mbed/targets/hal/rtl8721d/objects.h"
#include "rtl8721d_uart.h"
#include "tinyara/kmalloc.h"
#include "osdep_service.h"

/****************************************************************************
 * Preprocessor Definitions
 ****************************************************************************/
/*
 * If we are not using the serial driver for the console, then we
 * still must provide some minimal implementation of up_putc.
 */

#undef TTYS0_DEV
#undef TTYS1_DEV
#undef TTYS2_DEV

#undef UART0_ASSIGNED
#undef UART1_ASSIGNED
#undef UART2_ASSIGNED

/* Which UART with be ttyS0/console and which ttyS1? ttyS2 */
/* First pick the console and ttys0. This could be any of UART0-2 */
#if defined(CONFIG_UART0_SERIAL_CONSOLE)
#define CONSOLE_DEV             g_uart0port             /* UART0 is console */
#define TTYS0_DEV               g_uart0port             /* UART0 is ttyS0 */
#define CONSOLE                 UART0_DEV
#define UART0_ASSIGNED  1
#define HAVE_SERIAL_CONSOLE
#elif defined(CONFIG_UART1_SERIAL_CONSOLE)
#define CONSOLE_DEV             g_uart1port             /* UART1 is console */
#define TTYS0_DEV               g_uart1port             /* UART1 is ttyS0 */
#define CONSOLE                 UART3_DEV
#define UART1_ASSIGNED  1
#define HAVE_SERIAL_CONSOLE
#elif defined(CONFIG_UART2_SERIAL_CONSOLE)
#define CONSOLE_DEV             g_uart2port             /* UART2 is console */
#define TTYS0_DEV               g_uart2port             /* UART2 is ttyS0 */
#define CONSOLE                 UART2_DEV
#define UART2_ASSIGNED  1
#define HAVE_SERIAL_CONSOLE
#else
#undef CONSOLE_DEV                                              /* No console */
#if defined(CONFIG_RTL8721D_UART0)
#define TTYS0_DEV               g_uart0port             /* UART0 is ttyS0 */
#define UART0_ASSIGNED  1
#elif defined(CONFIG_RTL8721D_UART1)
#define TTYS0_DEV               g_uart1port             /* UART1 is ttyS0 */
#define UART1_ASSIGNED  1
#elif defined(CONFIG_RTL8721D_UART2)
#define TTYS0_DEV               g_uart2port             /* UART2 is ttyS0 */
#define UART2_ASSIGNED  1
#endif
#endif

/* Pick ttyS1. This could be any of UART0-2 excluding the console UART. */
#if defined(CONFIG_RTL8721D_UART0) && !defined(UART0_ASSIGNED)
#define TTYS1_DEV               g_uart0port             /* UART0 is ttyS1 */
#define UART0_ASSIGNED  1
#elif defined(CONFIG_RTL8721D_UART1) && !defined(UART1_ASSIGNED)
#define TTYS1_DEV               g_uart1port             /* UART1 is ttyS1 */
#define UART1_ASSIGNED  1
#elif defined(CONFIG_RTL8721D_UART2) && !defined(UART2_ASSIGNED)
#define TTYS1_DEV               g_uart2port             /* UART2 is ttyS1 */
#define UART2_ASSIGNED  1
#endif

/*
 * Pick ttyS2. This could be one of UART1-2. It can't be UART0 because that
 * was either assigned as ttyS0 or ttyS1. One of these could also be the
 * console
 */
#if defined(CONFIG_RTL8721D_UART1) && !defined(UART1_ASSIGNED)
#define TTYS2_DEV               g_uart1port             /* UART1 is ttyS2 */
#define UART1_ASSIGNED  1
#elif defined(CONFIG_RTL8721D_UART2) && !defined(UART2_ASSIGNED)
#define TTYS2_DEV               g_uart2port             /* UART2 is ttyS2 */
#define UART2_ASSIGNED  1
#endif
#define CHAR_TIMEOUT 6540

/****************************************************************************
 * Private Types
 ****************************************************************************/
/*
 * Available UARTs in AmebaD
 * UART0_DEV: KM4 uart0
 * UART1_DEV: KM4 uart1_bt
 * UART2_DEV: KM0 log uart
 * UART3_DEV: KM0 luart
 */

static serial_t* sdrv[MAX_UART_INDEX + 1] = {NULL, NULL, NULL, NULL, NULL}; //uart 0~3, uart2 is configured as log uart

struct rtl8721d_up_dev_s {
	uint8_t parity;				/* 0=none, 1=odd, 2=even */
	uint8_t bits;				/* Number of bits (7 or 8) */
	uint8_t stopbit;			/* Number of StopBit (1 or 2) */
	uint32_t baud;				/* Configured baud rate */
	uint32_t irq;				/* IRQ associated with this UART */
	PinName tx;					/* TX UART pin number */
	PinName rx;					/* RX UART pin number */
	PinName rts;				/* UART RTS pin number */
	PinName cts;				/* UART CTS pin number */
	uint8_t FlowControl;
	bool txint_enable;
	bool rxint_enable;
#ifdef CONFIG_SERIAL_IFLOWCONTROL
	uint8_t iflow:1;			/* input flow control (RTS) enabled */
#endif
#ifdef CONFIG_SERIAL_OFLOWCONTROL
	uint8_t oflow:1;			/* output flow control (CTS) enabled */
#endif
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int rtl8721d_up_setup(struct uart_dev_s *dev);
static void rtl8721d_up_shutdown(struct uart_dev_s *dev);
static int rtl8721d_up_attach(struct uart_dev_s *dev);
static void rtl8721d_up_detach(struct uart_dev_s *dev);
static int rtl8721d_up_interrupt(int irq, void *context, FAR void *arg);
static int rtl8721d_up_ioctl(FAR struct uart_dev_s *dev, int cmd, unsigned long arg);
static int rtl8721d_up_receive(struct uart_dev_s *dev, uint8_t *status);
static void rtl8721d_up_rxint(struct uart_dev_s *dev, bool enable);
static bool rtl8721d_up_rxavailable(struct uart_dev_s *dev);
static void rtl8721d_up_send(struct uart_dev_s *dev, int ch);
static void rtl8721d_up_txint(struct uart_dev_s *dev, bool enable);
static bool rtl8721d_up_txready(struct uart_dev_s *dev);
static bool rtl8721d_up_txempty(struct uart_dev_s *dev);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Serial driver UART operations */

static const struct uart_ops_s g_uart_ops = {
	.setup = rtl8721d_up_setup,
	.shutdown = rtl8721d_up_shutdown,
	.attach = rtl8721d_up_attach,
	.detach = rtl8721d_up_detach,
	.ioctl = rtl8721d_up_ioctl,
	.receive = rtl8721d_up_receive,
	.rxint = rtl8721d_up_rxint,
	.rxavailable = rtl8721d_up_rxavailable,
#ifdef CONFIG_SERIAL_IFLOWCONTROL
	.rxflowcontrol = NULL,
#endif
	.send = rtl8721d_up_send,
	.txint = rtl8721d_up_txint,
	.txready = rtl8721d_up_txready,
	.txempty = rtl8721d_up_txempty,
};

#ifdef CONFIG_RTL8721D_UART0
static char g_uart0rxbuffer[CONFIG_UART0_RXBUFSIZE];
static char g_uart0txbuffer[CONFIG_UART0_TXBUFSIZE];
#endif
#ifdef CONFIG_RTL8721D_UART1
static char g_uart1rxbuffer[CONFIG_UART1_RXBUFSIZE];
static char g_uart1txbuffer[CONFIG_UART1_TXBUFSIZE];
#endif
#ifdef CONFIG_RTL8721D_UART2
static char g_uart2rxbuffer[CONFIG_UART2_RXBUFSIZE];
static char g_uart2txbuffer[CONFIG_UART2_TXBUFSIZE];
#endif

#define RTL8721D_UART0_IRQ	(50)
#define RTL8721D_UART1_IRQ	(51)
#define RTL8721D_UART_LOG_IRQ	(19)
#define RTL8721D_UARTLP_IRQ	(25)

#ifdef CONFIG_RTL8721D_UART0
static struct rtl8721d_up_dev_s g_uart0priv = {

	.parity = CONFIG_UART0_PARITY,
	.bits = CONFIG_UART0_BITS,
#if (CONFIG_UART0_2STOP)
	.stopbit = 2,
#else
	.stopbit = 1,
#endif
	.baud = CONFIG_UART0_BAUD,
	.irq = RTL8721D_UART0_IRQ,
	.tx = PA_21,
	.rx = PA_22,
	.rts = PA_16,
	.cts = PA_17,
	.FlowControl = FlowControlNone,
	.txint_enable = false,
	.rxint_enable = false,
};

static uart_dev_t g_uart0port = {
	.isconsole = false,
	.recv = {
		.size = CONFIG_UART0_RXBUFSIZE,
		.buffer = g_uart0rxbuffer,
	},
	.xmit = {
		.size = CONFIG_UART0_TXBUFSIZE,
		.buffer = g_uart0txbuffer,
	},
	.ops = &g_uart_ops,
	.priv = &g_uart0priv,
};
#endif

#ifdef CONFIG_RTL8721D_UART1
static struct rtl8721d_up_dev_s g_uart1priv = {

	.parity = CONFIG_UART1_PARITY,
	.bits = CONFIG_UART1_BITS,
#if (CONFIG_UART1_2STOP)
	.stopbit = 2,
#else
	.stopbit = 1,
#endif
	.baud = CONFIG_UART1_BAUD,
	.irq = RTL8721D_UARTLP_IRQ,
	.tx = PA_12,
	.rx = PA_13,
	.FlowControl = FlowControlNone,
	.txint_enable = false,
	.rxint_enable = false,
};

static uart_dev_t g_uart1port = {
	.isconsole = false,
	.recv = {
		.size = CONFIG_UART1_RXBUFSIZE,
		.buffer = g_uart1rxbuffer,
	},
	.xmit = {
		.size = CONFIG_UART1_TXBUFSIZE,
		.buffer = g_uart1txbuffer,
	},
	.ops = &g_uart_ops,
	.priv = &g_uart1priv,
};
#endif

#ifdef CONFIG_RTL8721D_UART2
static struct rtl8721d_up_dev_s g_uart2priv = {

	.parity = CONFIG_UART2_PARITY,
	.bits = CONFIG_UART2_BITS,
#if (CONFIG_UART2_2STOP)
	.stopbit = 2,
#else
	.stopbit = 1,
#endif
	.baud = CONFIG_UART2_BAUD,
	.irq = RTL8721D_UART_LOG_IRQ,
	.tx = PA_7,
	.rx = PA_8,
	.FlowControl = FlowControlNone,
	.txint_enable = false,
	.rxint_enable = false,
};

static uart_dev_t g_uart2port = {
	.isconsole = true,
	.recv = {
		.size = CONFIG_UART2_RXBUFSIZE,
		.buffer = g_uart2rxbuffer,
	},
	.xmit = {
		.size = CONFIG_UART1_TXBUFSIZE,
		.buffer = g_uart2txbuffer,
	},
	.ops = &g_uart_ops,
	.priv = &g_uart2priv,
};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static u32 uart_index_get(PinName tx)
{
	if ((tx == _PA_12) || (tx == _PB_1) || (tx == _PA_26)) {
		return 3;
	} else if ((tx == _PA_18) || (tx == _PA_21) || (tx == _PB_9) || (tx == _PB_19)) {
		return 0;
	} else if (tx == _PA_7) {
		return 2;
	} else {
		assert_param(0);
	}
	return 3;
}

static void LOGUART_PutChar_RAM(u8 c)
{
	UART_TypeDef* LOGUART = CONSOLE;
	u32 CounterIndex = 0;

	if (ConfigDebugClose == 1)
		return;

	while(1) {
		CounterIndex++;
		if (CounterIndex >= CHAR_TIMEOUT)
			break;

		if (LOGUART->LSR & RUART_LINE_STATUS_REG_THRE)
			break;
	}
	LOGUART->RB_THR = c;
}

/****************************************************************************
 * Name: rtl8721d_up_setup
 *
 * Description:
 *   Configure the UART baud, bits, parity, fifos, etc. This
 *   method is called the first time that the serial port is
 *   opened.
 *
 ****************************************************************************/
static int rtl8721d_up_setup(struct uart_dev_s *dev)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);
	DEBUGASSERT(!sdrv[uart_index_get(priv->tx)]);
	sdrv[uart_index_get(priv->tx)] = (serial_t *)kmm_malloc(sizeof(serial_t));
	DEBUGASSERT(sdrv[uart_index_get(priv->tx)]);
	serial_init((serial_t *) sdrv[uart_index_get(priv->tx)], priv->tx, priv->rx);
	serial_baud(sdrv[uart_index_get(priv->tx)], priv->baud);
	serial_format(sdrv[uart_index_get(priv->tx)], priv->bits, priv->parity, priv->stopbit);
	serial_set_flow_control(sdrv[uart_index_get(priv->tx)], priv->FlowControl, priv->rts, priv->cts);

	return OK;
}

/****************************************************************************
 * Name: up_shutdown
 *
 * Description:
 *   Disable the UART.  This method is called when the serial
 *   port is closed
 *
 ****************************************************************************/

static void rtl8721d_up_shutdown(struct uart_dev_s *dev)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);
	DEBUGASSERT(sdrv[uart_index_get(priv->tx)]);
	serial_free(sdrv[uart_index_get(priv->tx)]);
	rtw_free(sdrv[uart_index_get(priv->tx)]);
	sdrv[uart_index_get(priv->tx)] = NULL;
}

/****************************************************************************
 * Name: up_attach
 *
 * Description:
 *   Configure the UART to operation in interrupt driven mode.  This method is
 *   called when the serial port is opened.  Normally, this is just after the
 *   the setup() method is called, however, the serial console may operate in
 *   a non-interrupt driven mode during the boot phase.
 *
 *   RX and TX interrupts are not enabled when by the attach method (unless the
 *   hardware supports multiple levels of interrupt enabling).  The RX and TX
 *   interrupts are not enabled until the txint() and rxint() methods are called.
 *
 ****************************************************************************/

//extern uint32_t uart_irqhandler(void *data);
void rtl8721d_uart_irq(uint32_t id, SerialIrq event)
{
	struct uart_dev_s *dev = (struct uart_dev_s *)id;
	if (event == RxIrq) {
		uart_recvchars(dev);
	}
	if (event == TxIrq) {
		uart_xmitchars(dev);
	}
}
static int rtl8721d_up_attach(struct uart_dev_s *dev)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	int ret = 0;
	DEBUGASSERT(priv);
	serial_irq_handler(sdrv[uart_index_get(priv->tx)], rtl8721d_uart_irq, (uint32_t) dev);
	return ret;
}

/****************************************************************************
 * Name: up_detach
 *
 * Description:
 *   Detach UART interrupts.  This method is called when the serial port is
 *   closed normally just before the shutdown method is called.  The exception is
 *   the serial console which is never shutdown.
 *
 ****************************************************************************/

static void rtl8721d_up_detach(struct uart_dev_s *dev)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);
	serial_irq_handler(sdrv[uart_index_get(priv->tx)], NULL, 0);
}

/****************************************************************************
 * Name: up_ioctl
 *
 * Description:
 *   All ioctl calls will be routed through this method
 *
 ****************************************************************************/

static int rtl8721d_up_ioctl(FAR struct uart_dev_s *dev, int cmd, unsigned long arg)
{
#if defined(CONFIG_SERIAL_TERMIOS)
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	int ret = OK;
	struct termios *termiosp = (struct termios *)arg;

	DEBUGASSERT(priv);
	switch (cmd) {
	case TCGETS:
		if (!termiosp) {
			return -EINVAL;
		}

		cfsetispeed(termiosp, priv->baud);

		termiosp->c_cflag = 0;

		if (priv->parity) {
			termiosp->c_cflag |= PARENB;
			if (priv->parity == RUART_ODD_PARITY) {
				termiosp->c_cflag |= PARODD;
			}
		}

		if (priv->stopbit == 2) {
			termiosp->c_cflag |= CSTOPB;
		}

		termiosp->c_cflag |= CS5 + ((priv->bits) - 5);
		break;

	case TCSETS:
		if (!termiosp) {
			return -EINVAL;
		}

		priv->bits = 5 + (termiosp->c_cflag & CSIZE);

		priv->parity = 0;		//ParityNoon
		if (termiosp->c_cflag & PARENB) {
			if (termiosp->c_cflag & PARODD) {
				priv->parity = ParityOdd;
			} else {
				priv->parity = ParityEven;
			}
		}
		if (!!(termiosp->c_cflag & CSTOPB)) {
			priv->stopbit = 2;
		} else {
			priv->stopbit = 1;
		}

		priv->baud = cfgetispeed(termiosp);
		if (sdrv[uart_index_get(priv->tx)]) {
			rtl8721d_up_shutdown(dev);
		}
		rtl8721d_up_setup(dev);
		rtl8721d_up_attach(dev);
		rtl8721d_up_txint(dev, priv->txint_enable);
		rtl8721d_up_rxint(dev, priv->rxint_enable);
		break;

	case TIOCS_AVAIL:
		ret = rtl8721d_up_rxavailable(dev);
		break;
	case TIOCS_READY:
		ret = rtl8721d_up_txready(dev);
		break;
	case TIOCS_EMPTY:
		ret = rtl8721d_up_txempty(dev);
		break;
	default:
		ret = -ENOTTY;
		break;
	}
#else
	int ret = -ENOTTY;
#endif
	return ret;
}

/****************************************************************************
 * Name: up_receive
 *
 * Description:
 *   Called (usually) from the interrupt level to receive one
 *   character from the UART.  Error bits associated with the
 *   receipt are provided in the return 'status'.
 *
 ****************************************************************************/

static int rtl8721d_up_receive(struct uart_dev_s *dev, uint8_t *status)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	uint32_t rxd;

	DEBUGASSERT(priv);
	rxd = serial_getc(sdrv[uart_index_get(priv->tx)]);
	*status = rxd;

	return rxd & 0xff;
}

/****************************************************************************
 * Name: up_rxint
 *
 * Description:
 *   Call to enable or disable RX interrupts
 *
 ****************************************************************************/
static void rtl8721d_up_rxint(struct uart_dev_s *dev, bool enable)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);
	priv->rxint_enable = enable;
	serial_irq_set(sdrv[uart_index_get(priv->tx)], RxIrq, enable);	// 1= ENABLE
}

/****************************************************************************
 * Name: up_rxavailable
 *
 * Description:
 *   Return true if the receive fifo is not empty
 *
 ****************************************************************************/

static bool rtl8721d_up_rxavailable(struct uart_dev_s *dev)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);
	return (serial_readable(sdrv[uart_index_get(priv->tx)]));
}

/****************************************************************************
 * Name: up_send
 *
 * Description:
 *   This method will send one byte on the UART
 *
 ****************************************************************************/

static void rtl8721d_up_send(struct uart_dev_s *dev, int ch)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);
	/*write one byte to tx fifo*/
	serial_putc(sdrv[uart_index_get(priv->tx)], ch);
}

/****************************************************************************
 * Name: up_txint
 *
 * Description:
 *   Call to enable or disable TX interrupts
 *
 ****************************************************************************/

static void rtl8721d_up_txint(struct uart_dev_s *dev, bool enable)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);
	priv->txint_enable = enable;
	serial_irq_set(sdrv[uart_index_get(priv->tx)], TxIrq, enable);
	if (enable)
		UART_INTConfig(UART_DEV_TABLE[uart_index_get(priv->tx)].UARTx, RUART_IER_ETBEI, ENABLE);
}

/****************************************************************************
 * Name: up_txready
 *
 * Description:
 *   Return true if the tranmsit fifo is not full
 *
 ****************************************************************************/

static bool rtl8721d_up_txready(struct uart_dev_s *dev)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);

#if defined(CONFIG_UART0_SERIAL_CONSOLE)
	if (uart_index_get(priv->tx) == 0){
		while (!serial_writable(sdrv[uart_index_get(priv->tx)]));
		return true;
	}
	else
#elif defined(CONFIG_UART1_SERIAL_CONSOLE)
	if (uart_index_get(priv->tx) == 3){
		while (!serial_writable(sdrv[uart_index_get(priv->tx)]));
		return true;
	}
	else
#elif defined(CONFIG_UART2_SERIAL_CONSOLE)
	if (uart_index_get(priv->tx) == 2){
		while (!serial_writable(sdrv[uart_index_get(priv->tx)]));
		return true;
	}
	else
#endif
	{
		return (serial_writable(sdrv[uart_index_get(priv->tx)]));
	}
}

/****************************************************************************
 * Name: up_txempty
 *
 * Description:
 *   Return true if the transmit fifo is empty
 *
 ****************************************************************************/

static bool rtl8721d_up_txempty(struct uart_dev_s *dev)
{
	struct rtl8721d_up_dev_s *priv = (struct rtl8721d_up_dev_s *)dev->priv;
	DEBUGASSERT(priv);
	while (!serial_writable(sdrv[uart_index_get(priv->tx)])) ;
	return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef USE_SERIALDRIVER
/****************************************************************************
 * Name: up_serialinit
 *
 * Description:
 *   Register serial console and serial ports.  This assumes
 *   that up_earlyserialinit was called previously.
 *
 ****************************************************************************/

void up_serialinit(void)
{
	UART_WaitBusy(UART2_DEV, 100);
	UART_DeInit(UART2_DEV);
	UART_ClearRxFifo(UART2_DEV);
	UART_ClearTxFifo(UART2_DEV);

#ifdef CONSOLE_DEV
	CONSOLE_DEV.isconsole = true;
	rtl8721d_up_setup(&CONSOLE_DEV);

	/* Register the console */
	uart_register("/dev/console", &CONSOLE_DEV);
#endif

	/* Register all UARTs */
#ifdef TTYS0_DEV
	uart_register("/dev/ttyS0", &TTYS0_DEV);
#endif
#ifdef TTYS1_DEV
	uart_register("/dev/ttyS1", &TTYS1_DEV);
#endif
#ifdef TTYS2_DEV
	uart_register("/dev/ttyS2", &TTYS2_DEV);
#endif

}

/****************************************************************************
 * Name: up_lowputc
 *
 * Description:
 *   Output one byte on the serial console
 *
 * Input Parameters:
 *   ch - chatacter to output
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
void up_lowputc(char ch)
{
	LOGUART_PutChar_RAM(ch);
}

/****************************************************************************
 * Name: up_lowgetc
 *
 * Description:
 *   Read one byte from the serial console
 *
 * Input Parameters:
 *   none
 *
 * Returned Value:
 *   int value, -1 if error, 0~255 if byte successfully read
 *
 ****************************************************************************/
int up_lowgetc(void)
{
	uint8_t rxd;
	LOGUART_SetIMR(0);
	rxd = LOGUART_GetChar(_TRUE);
	LOGUART_SetIMR(1);
	return rxd & 0xff;
}

/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Output one byte on the serial console
 *
 * Input Parameters:
 *   ch - chatacter to output
 *
 * Returned Value:
 *  sent character
 *
 ****************************************************************************/
int up_putc(int ch)
{
	/* Check for LF */

	if (ch == '\n') {
		/* Add CR */

		up_lowputc('\r');
	}

	up_lowputc(ch);
	return ch;
}

/****************************************************************************
 * Name: up_getc
 *
 * Description:
 *   Read one byte from the serial console
 *
 * Input Parameters:
 *   none
 *
 * Returned Value:
 *   int value, -1 if error, 0~255 if byte successfully read
 *
 ****************************************************************************/
int up_getc(void)
{
	int ch;
	ch = up_lowgetc();
	return ch;
}

#else							/* USE_SERIALDRIVER */
/****************************************************************************
 * Name: up_putc
 *
 * Description:
 *   Output one byte on the serial console
 *
 * Input Parameters:
 *   ch - chatacter to output
 *
 * Returned Value:
 *  sent character
 *
 ****************************************************************************/
int up_putc(int ch)
{
	/* Check for LF */

	if (ch == '\n') {
		/* Add CR */

		up_lowputc('\r');
	}
	up_lowputc(ch);

	return ch;
}

/****************************************************************************
 * Name: up_getc
 *
 * Description:
 *   Read one byte from the serial console
 *
 * Input Parameters:
 *   none
 *
 * Returned Value:
 *   int value, -1 if error, 0~255 if byte successfully read
 *
 ****************************************************************************/
int up_getc(void)
{
	int ch;
	ch = up_lowgetc();
	return ch;
}
#endif							/* USE_SERIALDRIVER */
