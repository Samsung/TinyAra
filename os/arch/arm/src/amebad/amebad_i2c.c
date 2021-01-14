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

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/arch.h>
#include <tinyara/irq.h>
#include <tinyara/clock.h>
#include <tinyara/semaphore.h>
#include <tinyara/i2c.h>
#include <tinyara/mm/mm.h>

#include "mbed/targets/hal/rtl8721d/PinNames.h"

#include <arch/board/board.h>

#include "up_arch.h"

#include "amebad_i2c.h"

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

/* Configuration ********************************************************************/
/* CONFIG_I2C_POLLED may be set so that I2C interrupts will not be used.  Instead,
 * CPU-intensive polling will be used.
 */

/* Interrupt wait timeout in seconds and milliseconds */

#if !defined(CONFIG_AMEBAD_I2CTIMEOSEC) && !defined(CONFIG_AMEBAD_I2CTIMEOMS)
#define CONFIG_AMEBAD_I2CTIMEOSEC 0
#define CONFIG_AMEBAD_I2CTIMEOMS  500	/* Default is 500 milliseconds */
#elif !defined(CONFIG_AMEBAD_I2CTIMEOSEC)
#define CONFIG_AMEBAD_I2CTIMEOSEC 0	/* User provided milliseconds */
#elif !defined(CONFIG_AMEBAD_I2CTIMEOMS)
#define CONFIG_AMEBAD_I2CTIMEOMS  0	/* User provided seconds */
#endif

/* Interrupt wait time timeout in system timer ticks */

#ifndef CONFIG_AMEBAD_I2CTIMEOTICKS
#define CONFIG_AMEBAD_I2CTIMEOTICKS \
	(SEC2TICK(CONFIG_AMEBAD_I2CTIMEOSEC) + MSEC2TICK(CONFIG_AMEBAD_I2CTIMEOMS))
#endif

#ifndef CONFIG_AMEBAD_I2C_DYNTIMEO_STARTSTOP
#define CONFIG_AMEBAD_I2C_DYNTIMEO_STARTSTOP TICK2USEC(CONFIG_AMEBAD_I2CTIMEOTICKS)
#endif

/* Debug ****************************************************************************/

/* I2C event trace logic.  NOTE:  trace uses the internal, non-standard, low-level
 * debug interface syslog() but does not require that any other debug
 * is enabled.
 */

#ifndef CONFIG_I2C_TRACE
#define amebad_i2c_tracereset(p)
#define amebad_i2c_tracenew(p, s)
#define amebad_i2c_traceevent(p, e, a)
#define amebad_i2c_tracedump(p)
#endif

#ifndef CONFIG_I2C_NTRACE
#define CONFIG_I2C_NTRACE 32
#endif

#define AMEBAD_I2C_MASTER    1
#define AMEBAD_I2C_SLAVE     2

#define I2C_MASTER_DEVICE		1
#define I2C_SLAVE_DEVICE		2

#define MKI2C_OUTPUT(p) (((p) & GPIO_PADMUX_MASK) | \
			 IOMUX_OPENDRAIN | IOMUX_DRIVE_33OHM | \
			 IOMUX_SLEW_SLOW | (5 << GPIO_ALT_SHIFT) | \
			 IOMUX_PULL_NONE | GPIO_OUTPUT_ONE)

#define MKI2C_INPUT(p) (((p) & GPIO_PADMUX_MASK) | \
			IOMUX_DRIVE_HIZ | IOMUX_SLEW_SLOW | \
			IOMUX_CMOS_INPUT | (5 << GPIO_ALT_SHIFT) | \
			IOMUX_PULL_NONE)

#undef i2cerr
#undef i2cinfo
#if defined(DEBUG_IMX_I2C)
#define i2cinfo(format, ...)   printf(format, ##__VA_ARGS__)
#define i2cerr(format, ...)   printf(format, ##__VA_ARGS__)
#else
#define i2cinfo(format, ...)
//#define nerr(format, ...)
#endif
#define i2cerr(format, ...)   printf(format, ##__VA_ARGS__)

/************************************************************************************
 * Private Types
 ************************************************************************************/

/* Interrupt state */

enum amebad_intstate_e {
	INTSTATE_IDLE = 0,			/* No I2C activity */
	INTSTATE_WAITING,			/* Waiting for completion of interrupt activity */
	INTSTATE_DONE,				/* Interrupt activity complete */
};

/* Trace events */

#ifdef CONFIG_I2C_TRACE
enum amebad_trace_e {
	I2CEVENT_NONE = 0,			/* No events have occurred with this status */
	I2CEVENT_SENDADDR,			/* Start/Master bit set and address sent, param = msgc */
	I2CEVENT_SENDBYTE,			/* Send byte, param = dcnt */
	I2CEVENT_RCVBYTE,			/* Read more dta, param = dcnt */
	I2CEVENT_NOSTART,			/* BTF on last byte with no restart, param = msgc */
	I2CEVENT_STARTRESTART,		/* Last byte sent, re-starting, param = msgc */
	I2CEVENT_STOP,				/* Last byte sten, send stop, param = 0 */
	I2CEVENT_ERROR				/* Error occurred, param = 0 */
};

/* Trace data */

struct amebad_trace_s {
	uint32_t status;			/* I2C 32-bit SR2|SR1 status */
	uint32_t count;				/* Interrupt count when status change */
	enum amebad_intstate_e event;	/* Last event that occurred with this status */
	uint32_t parm;				/* Parameter associated with the event */
	clock_t time;				/* First of event or first status */
};
#endif

/* I2C Device hardware configuration */

struct amebad_i2c_config_s {
	//uint32_t base;				/* I2C base address */
	//uint16_t busy_idle;			/* I2C Bus Idle Timeout */
	//uint8_t filtscl;			/* Glitch Filter for SCL pin */
	//uint8_t filtsda;			/* Glitch Filter for SDA pin */
	uint32_t scl_pin;			/* GPIO configuration for SCL as SCL */
	uint32_t sda_pin;			/* GPIO configuration for SDA as SDA */
	uint8_t mode;				/* Master or Slave mode */
#ifndef CONFIG_I2C_POLLED
	uint32_t irq;				/* Event IRQ */
#endif
};

struct i2c_m {
	uint32_t i2c_idx;
	I2C_TypeDef * I2Cx;
};
typedef struct i2c_m i2c_t;

/* I2C Device Private Data */

struct amebad_i2c_priv_s {
	const struct i2c_ops_s *ops;	/* Standard I2C operations */
	const struct amebad_i2c_config_s *config;	/* Port configuration */
	int refs;					/* Referernce count */
	sem_t sem_excl;				/* Mutual exclusion semaphore */
#ifndef CONFIG_I2C_POLLED
	sem_t sem_isr;				/* Interrupt wait semaphore */
#endif
	volatile uint8_t intstate;	/* Interrupt handshake (see enum amebad_intstate_e) */
	int address;				/* Address used in this instantiation */

	uint8_t msgc;				/* Message count */
	struct i2c_msg_s *msgv;		/* Message list */
	uint8_t *ptr;				/* Current message buffer */
	uint32_t frequency;			/* Current I2C frequency */
	int dcnt;					/* Current message length */
	uint16_t flags;				/* Current message flags */

	i2c_t *i2c_object;

	/* I2C trace support */

#ifdef CONFIG_I2C_TRACE
	int tndx;					/* Trace array index */
	clock_t start_time;			/* Time when the trace was started */

	/* The actual trace data */

	struct amebad_trace_s trace[CONFIG_I2C_NTRACE];
#endif

	uint32_t status;			/* End of transfer SR2|SR1 status */
};

/************************************************************************************
 * Private Function Prototypes
 ************************************************************************************/

static inline uint32_t amebad_i2c_getreg(FAR struct amebad_i2c_priv_s *priv, uint16_t offset);
static inline void amebad_i2c_putreg(FAR struct amebad_i2c_priv_s *priv, uint16_t offset, uint32_t value);
static inline void amebad_i2c_modifyreg(FAR struct amebad_i2c_priv_s *priv, uint16_t offset, uint32_t clearbits, uint32_t setbits);
static inline void amebad_i2c_sem_wait(FAR struct amebad_i2c_priv_s *priv);

#ifdef CONFIG_AMEBAD_I2C_DYNTIMEO
static useconds_t amebad_i2c_tousecs(int msgc, FAR struct i2c_msg_s *msgs);
#endif							/* CONFIG_AMEBAD_I2C_DYNTIMEO */

static inline int amebad_i2c_sem_waitdone(FAR struct amebad_i2c_priv_s *priv);
static inline void amebad_i2c_sem_waitstop(FAR struct amebad_i2c_priv_s *priv);
static inline void amebad_i2c_sem_post(FAR struct amebad_i2c_priv_s *priv);
static inline void amebad_i2c_sem_init(FAR struct amebad_i2c_priv_s *priv);
static inline void amebad_i2c_sem_destroy(FAR struct amebad_i2c_priv_s *priv);

#ifdef CONFIG_I2C_TRACE
static void amebad_i2c_tracereset(FAR struct amebad_i2c_priv_s *priv);
static void amebad_i2c_tracenew(FAR struct amebad_i2c_priv_s *priv, uint32_t status);
static void amebad_i2c_traceevent(FAR struct amebad_i2c_priv_s *priv, enum amebad_trace_e event, uint32_t parm);
static void amebad_i2c_tracedump(FAR struct amebad_i2c_priv_s *priv);
#endif							/* CONFIG_I2C_TRACE */

static void amebad_i2c_setclock(FAR struct amebad_i2c_priv_s *priv, uint32_t frequency);
static inline void amebad_i2c_sendstart(FAR struct amebad_i2c_priv_s *priv, uint8_t address);
static inline void amebad_i2c_sendstop(FAR struct amebad_i2c_priv_s *priv);
static inline uint32_t amebad_i2c_getstatus(FAR struct amebad_i2c_priv_s *priv);

static int amebad_i2c_isr_process(struct amebad_i2c_priv_s *priv);

#ifndef CONFIG_I2C_POLLED
static int amebad_i2c_isr(int irq, void *context, FAR void *arg);
#endif							/* !CONFIG_I2C_POLLED */

static int amebad_i2c_init(FAR struct amebad_i2c_priv_s *priv);
static int amebad_i2c_deinit(FAR struct amebad_i2c_priv_s *priv);
#ifdef CONFIG_I2C_TRANSFER
static int amebad_i2c_transfer(FAR struct i2c_dev_s *dev, FAR struct i2c_msg_s *msgs, int count);
#endif
#ifdef CONFIG_I2C_RESET
static int amebad_i2c_reset(FAR struct i2c_master_s *dev);
#endif

static int amebad_i2c_process(FAR struct i2c_dev_s *dev, FAR struct i2c_msg_s *msgs, int count);
static uint32_t amebad_i2c_setfrequency(FAR struct i2c_dev_s *dev, uint32_t frequency);
static int amebad_i2c_setaddress(FAR struct i2c_dev_s *dev, int addr, int nbits);
static int amebad_i2c_write(FAR struct i2c_dev_s *dev, const uint8_t *buffer, int buflen);
static int amebad_i2c_read(FAR struct i2c_dev_s *dev, uint8_t *buffer, int buflen);
#ifdef CONFIG_I2C_WRITEREAD
static int amebad_i2c_writeread(FAR struct i2c_dev_s *dev, const uint8_t *wbuffer, int wbuflen, uint8_t *buffer, int buflen);
#endif

/************************************************************************************
 * Private Data
 ************************************************************************************/

/* Trace events strings */

#ifdef CONFIG_I2C_TRACE
static const char *g_trace_names[] = {
	"NONE           ",
	"SENDADDR       ",
	"SENDBYTE       ",
	"RCVBYTE        ",
	"NOSTART        ",
	"START/RESTART  ",
	"STOP           ",
	"ERROR          "
};
#endif

/* I2C interface */

static const struct i2c_ops_s amebad_i2c_ops = {
	.setfrequency = amebad_i2c_setfrequency,
	.setaddress = amebad_i2c_setaddress,
	.write = amebad_i2c_write,
	.read = amebad_i2c_read,
#ifdef CONFIG_I2C_WRITEREAD
	.writeread = amebad_i2c_writeread,
#endif
#ifdef CONFIG_I2C_TRANSFER
	.transfer = amebad_i2c_transfer,
#endif
#ifdef CONFIG_I2C_SLAVE
	.setownaddress = amebad_i2c_setaddress, //amebad_i2c_setownaddress,
	//.registercallback = amebad_i2c_registercallback
#endif
};

/* I2C device structures */

static const struct amebad_i2c_config_s amebad_i2c1_config = {
	//.base = AMEBAD_I2C1_BASE,
	//.busy_idle = CONFIG_I2C1_BUSYIDLE,
	//.filtscl = CONFIG_I2C1_FILTSCL,
	//.filtsda = CONFIG_I2C1_FILTSDA,
	.scl_pin = PA_25,
	.sda_pin = PA_26,
#ifndef CONFIG_I2C_SLAVE
	.mode = AMEBAD_I2C_MASTER,
#else
	.mode = AMEBAD_I2C_SLAVE,
#endif
#ifndef CONFIG_I2C_POLLED
	.irq = AMEBAD_IRQ_I2C0,
#endif
};

static struct amebad_i2c_priv_s amebad_i2c1_priv = {
	.ops = &amebad_i2c_ops,
	.config = &amebad_i2c1_config,
	.refs = 0,
	.intstate = INTSTATE_IDLE,
	.msgc = 0,
	.msgv = NULL,
	.ptr = NULL,
	.dcnt = 0,
	.flags = 0,
	.status = 0
};

/************************************************************************************
 * Private Functions
 ************************************************************************************/

/************************************************************************************
 * Name: amebad_i2c_getreg
 *
 * Description:
 *   Get a 32-bit register value by offset
 *
 ************************************************************************************/

static inline uint32_t amebad_i2c_getreg(FAR struct amebad_i2c_priv_s *priv, uint16_t offset)
{
	//return getreg32(priv->config->base + offset);
	return 0;
}

/************************************************************************************
 * Name: amebad_i2c_putreg
 *
 * Description:
 *  Put a 32-bit register value by offset
 *
 ************************************************************************************/

static inline void amebad_i2c_putreg(FAR struct amebad_i2c_priv_s *priv, uint16_t offset, uint32_t value)
{
	//putreg32(value, priv->config->base + offset);
}

/************************************************************************************
 * Name: amebad_i2c_modifyreg
 *
 * Description:
 *   Modify a 32-bit register value by offset
 *
 ************************************************************************************/

static inline void amebad_i2c_modifyreg(FAR struct amebad_i2c_priv_s *priv, uint16_t offset, uint32_t clearbits, uint32_t setbits)
{
	//modifyreg32(priv->config->base + offset, clearbits, setbits);
}

/************************************************************************************
 * Name: amebad_i2c_sem_wait
 *
 * Description:
 *   Take the exclusive access, waiting as necessary
 *
 ************************************************************************************/

static inline void amebad_i2c_sem_wait(FAR struct amebad_i2c_priv_s *priv)
{
	int ret;

	do {
		/* Take the semaphore (perhaps waiting) */

		ret = sem_wait(&priv->sem_excl);

		/* The only case that an error should occur here is if the wait was
		 * awakened by a signal.
		 */

		DEBUGASSERT(ret == OK || errno == EINTR);
	} while (errno == EINTR);
}

/************************************************************************************
 * Name: amebad_i2c_tousecs
 *
 * Description:
 *   Return a micro-second delay based on the number of bytes left to be processed.
 *
 ************************************************************************************/

#ifdef CONFIG_AMEBAD_I2C_DYNTIMEO
static useconds_t amebad_i2c_tousecs(int msgc, FAR struct i2c_msg_s *msgs)
{
	size_t bytecount = 0;
	int i;

	/* Count the number of bytes left to process */

	for (i = 0; i < msgc; i++) {
		bytecount += msgs[i].length;
	}

	/* Then return a number of microseconds based on a user provided scaling
	 * factor.
	 */

	return (useconds_t)(CONFIG_AMEBAD_I2C_DYNTIMEO_USECPERBYTE * bytecount);
}
#endif

/************************************************************************************
 * Name: amebad_i2c_sem_waitdone
 *
 * Description:
 *   Wait for a transfer to complete
 *
 ************************************************************************************/

#ifndef CONFIG_I2C_POLLED
static inline int amebad_i2c_sem_waitdone(FAR struct amebad_i2c_priv_s *priv)
{
	struct timespec abstime;
	irqstate_t flags;
	uint32_t regval;
	int ret;

	flags = irqsave();

	/* Enable I2C interrupts */
	up_enable_irq(priv->config->irq);

	/* Signal the interrupt handler that we are waiting.  NOTE:  Interrupts
	 * are currently disabled but will be temporarily re-enabled below when
	 * sem_timedwait() sleeps.
	 */

	priv->intstate = INTSTATE_WAITING;
	do {
		/* Get the current time */

		(void)clock_gettime(CLOCK_REALTIME, &abstime);

		/* Calculate a time in the future */

#if CONFIG_AMEBAD_I2CTIMEOSEC > 0
		abstime.tv_sec += CONFIG_AMEBAD_I2CTIMEOSEC;
#endif

		/* Add a value proportional to the number of bytes in the transfer */

#ifdef CONFIG_AMEBAD_I2C_DYNTIMEO
		abstime.tv_nsec += 1000 * amebad_i2c_tousecs(priv->msgc, priv->msgv);
		if (abstime.tv_nsec > 1000 * 1000 * 1000) {
			abstime.tv_sec++;
			abstime.tv_nsec -= 1000 * 1000 * 1000;
		}
#elif CONFIG_AMEBAD_I2CTIMEOMS > 0
		abstime.tv_nsec += CONFIG_AMEBAD_I2CTIMEOMS * 1000 * 1000;
		if (abstime.tv_nsec > 1000 * 1000 * 1000) {
			abstime.tv_sec++;
			abstime.tv_nsec -= 1000 * 1000 * 1000;
		}
#endif

		/* Wait until either the transfer is complete or the timeout expires */

		ret = sem_timedwait(&priv->sem_isr, &abstime);
		if (ret != OK && errno != EINTR) {
			/* Break out of the loop on irrecoverable errors.  This would
			 * include timeouts and mystery errors reported by sem_timedwait.
			 * NOTE that we try again if we are awakened by a signal (EINTR).
			 */

			break;
		}
	}

	/* Loop until the interrupt level transfer is complete. */

	while (priv->intstate != INTSTATE_DONE);

	/* Set the interrupt state back to IDLE */

	priv->intstate = INTSTATE_IDLE;

	/* Disable I2C interrupts */

	up_disable_irq(priv->config->irq);

	irqrestore(flags);
	return ret;
}
#else

static inline int amebad_i2c_sem_waitdone(FAR struct amebad_i2c_priv_s *priv)
{
	uint32_t timeout;
	clock_t start;
	clock_t elapsed;
	int ret;

	/* Get the timeout value */

#ifdef CONFIG_AMEBAD_I2C_DYNTIMEO
	timeout = USEC2TICK(amebad_i2c_tousecs(priv->msgc, priv->msgv));
#else
	timeout = CONFIG_AMEBAD_I2CTIMEOTICKS;
#endif

	/* Signal the interrupt handler that we are waiting.  NOTE:  Interrupts
	 * are currently disabled but will be temporarily re-enabled below when
	 * sem_timedwait() sleeps.
	 */

	priv->intstate = INTSTATE_WAITING;
	start = clock_systimer();

	do {
		/* Poll by simply calling the timer interrupt handler until it
		 * reports that it is done.
		 */

		ret = amebad_i2c_isr_process(priv);

		/* Calculate the elapsed time */

		elapsed = clock_systimer() - start;
	}

	/* Loop until the transfer is complete. */

	while (priv->intstate != INTSTATE_DONE && elapsed < timeout);

	i2cinfo("intstate: %d elapsed: %d threshold: %d status: %08x\n", priv->intstate, elapsed, timeout, priv->status);

	/* Set the interrupt state back to IDLE */

	ret = priv->intstate == INTSTATE_DONE ? ret : -ETIMEDOUT;
	priv->intstate = INTSTATE_IDLE;
	return ret;
}
#endif

/************************************************************************************
 * Name: amebad_i2c_sem_waitstop
 *
 * Description:
 *   Wait for a STOP to complete
 *
 ************************************************************************************/

static inline void amebad_i2c_sem_waitstop(FAR struct amebad_i2c_priv_s *priv)
{
}

/************************************************************************************
 * Name: amebad_i2c_sem_post
 *
 * Description:
 *   Release the mutual exclusion semaphore
 *
 ************************************************************************************/

static inline void amebad_i2c_sem_post(struct amebad_i2c_priv_s *priv)
{
	sem_post(&priv->sem_excl);
}

/************************************************************************************
 * Name: amebad_i2c_sem_init
 *
 * Description:
 *   Initialize semaphores
 *
 ************************************************************************************/

static inline void amebad_i2c_sem_init(FAR struct amebad_i2c_priv_s *priv)
{
	sem_init(&priv->sem_excl, 0, 1);

#ifndef CONFIG_I2C_POLLED
	/* This semaphore is used for signaling and, hence, should not have
	 * priority inheritance enabled.
	 */

	sem_init(&priv->sem_isr, 0, 0);
	sem_setprotocol(&priv->sem_isr, SEM_PRIO_NONE);
#endif
}

/************************************************************************************
 * Name: amebad_i2c_sem_destroy
 *
 * Description:
 *   Destroy semaphores.
 *
 ************************************************************************************/

static inline void amebad_i2c_sem_destroy(FAR struct amebad_i2c_priv_s *priv)
{
	sem_destroy(&priv->sem_excl);
#ifndef CONFIG_I2C_POLLED
	sem_destroy(&priv->sem_isr);
#endif
}

/************************************************************************************
 * Name: amebad_i2c_traceclear
 *
 * Description:
 *   I2C trace instrumentation
 *
 ************************************************************************************/

#ifdef CONFIG_I2C_TRACE
static void amebad_i2c_traceclear(FAR struct amebad_i2c_priv_s *priv)
{
	struct amebad_trace_s *trace = &priv->trace[priv->tndx];

	trace->status = 0;			/* I2C 32-bit SR2|SR1 status */
	trace->count = 0;			/* Interrupt count when status change */
	trace->event = I2CEVENT_NONE;	/* Last event that occurred with this status */
	trace->parm = 0;			/* Parameter associated with the event */
	trace->time = 0;			/* Time of first status or event */
}

static void amebad_i2c_tracereset(FAR struct amebad_i2c_priv_s *priv)
{
	/* Reset the trace info for a new data collection */

	priv->tndx = 0;
	priv->start_time = clock_systimer();
	amebad_i2c_traceclear(priv);
}

static void amebad_i2c_tracenew(FAR struct amebad_i2c_priv_s *priv, uint32_t status)
{
	struct amebad_trace_s *trace = &priv->trace[priv->tndx];

	/* Is the current entry uninitialized?  Has the status changed? */

	if (trace->count == 0 || status != trace->status) {
		/* Yes.. Was it the status changed?  */

		if (trace->count != 0) {
			/* Yes.. bump up the trace index (unless we are out of trace entries) */

			if (priv->tndx >= (CONFIG_I2C_NTRACE - 1)) {
				i2cerr("ERROR: Trace table overflow\n");
				return;
			}

			priv->tndx++;
			trace = &priv->trace[priv->tndx];
		}

		/* Initialize the new trace entry */

		amebad_i2c_traceclear(priv);
		trace->status = status;
		trace->count = 1;
		trace->time = clock_systimer();
	} else {
		/* Just increment the count of times that we have seen this status */

		trace->count++;
	}
}

static void amebad_i2c_traceevent(FAR struct amebad_i2c_priv_s *priv, enum amebad_trace_e event, uint32_t parm)
{
	struct amebad_trace_s *trace;

	if (event != I2CEVENT_NONE) {
		trace = &priv->trace[priv->tndx];

		/* Initialize the new trace entry */

		trace->event = event;
		trace->parm = parm;

		/* Bump up the trace index (unless we are out of trace entries) */

		if (priv->tndx >= (CONFIG_I2C_NTRACE - 1)) {
			i2cerr("ERROR: Trace table overflow\n");
			return;
		}

		priv->tndx++;
		amebad_i2c_traceclear(priv);
	}
}

static void amebad_i2c_tracedump(FAR struct amebad_i2c_priv_s *priv)
{
	struct amebad_trace_s *trace;
	int i;

	syslog(LOG_DEBUG, "Elapsed time: %ld\n", (long)(clock_systimer() - priv->start_time));

	for (i = 0; i <= priv->tndx; i++) {
		trace = &priv->trace[i];
		syslog(LOG_DEBUG, "%2d. STATUS: %08x COUNT: %3d EVENT: %s(%2d) PARM: %08x TIME: %d\n",
		       i + 1, trace->status, trace->count, g_trace_names[trace->event], trace->event,
		       trace->parm, trace->time - priv->start_time);
	}
}
#endif							/* CONFIG_I2C_TRACE */

/************************************************************************************
 * Name: amebad_i2c_setclock
 *
 * Description:
 *   Set the I2C clock
 *
 ************************************************************************************/

static void amebad_i2c_setclock(FAR struct amebad_i2c_priv_s *priv, uint32_t frequency)
{
}

/************************************************************************************
 * Name: amebad_i2c_sendstart
 *
 * Description:
 *   Send the START conditions/force Master mode
 *
 ************************************************************************************/

static inline void amebad_i2c_sendstart(FAR struct amebad_i2c_priv_s *priv, uint8_t address)
{
}

/************************************************************************************
 * Name: amebad_i2c_sendstop
 *
 * Description:
 *   Send the STOP conditions
 *
 ************************************************************************************/

static inline void amebad_i2c_sendstop(FAR struct amebad_i2c_priv_s *priv)
{
}

/************************************************************************************
 * Name: amebad_i2c_getstatus
 *
 * Description:
 *   Get 32-bit status
 *
 ************************************************************************************/

static inline uint32_t amebad_i2c_getstatus(FAR struct amebad_i2c_priv_s *priv)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_i2c_isr_process
 *
 * Description:
 *  Common Interrupt Service Routine
 *
 ************************************************************************************/

static int amebad_i2c_isr_process(struct amebad_i2c_priv_s *priv)
{
	int ret = 0;

	struct i2c_msg_s *w_msgv = priv->msgv;
#ifdef CONFIG_I2C_WRITEREAD
	struct i2c_msg_s *r_msgv = ++priv->msgv;
	uint8_t read_restart;
	uint8_t write_restart;

	if ((w_msgv->flags & I2C_M_READ) == 0) {

		i2cinfo("i2c writing");
#ifdef CONFIG_I2C_SLAVE

		i2c_slave_read(priv->i2c_object, &read_restart, 1);
		i2c_slave_set_for_rd_req(priv->i2c_object, 1);
		ret = i2c_slave_write(priv->i2c_object, w_msgv->buffer, w_msgv->length);
#else
		ret = rtk_i2c_write(priv->i2c_object, w_msgv->addr, &write_restart, 1, 0);
		ret = rtk_i2c_write(priv->i2c_object, w_msgv->addr, w_msgv->buffer, w_msgv->length, 1);
#endif
        }
	if ((r_msgv->flags & I2C_M_READ) != 0) {

		i2cinfo("i2c reading");
#ifdef CONFIG_I2C_SLAVE

		ret = i2c_slave_read(priv->i2c_object, r_msgv->buffer, r_msgv->length);
#else
		rtk_i2c_write(priv->i2c_object, r_msgv->addr, &write_restart, 1, 0);
		ret = rtk_i2c_read(priv->i2c_object, r_msgv->addr, r_msgv->buffer, r_msgv->length, 1);
#endif
	}

#else /* CONFIG_I2C_WRITEREAD */

	if ((w_msgv->flags & I2C_M_READ) == 0) {

		i2cinfo("i2c writing");
#ifdef CONFIG_I2C_SLAVE

		i2c_slave_set_for_rd_req(priv->i2c_object, 1);
		ret = i2c_slave_write(priv->i2c_object, w_msgv->buffer, w_msgv->length);
#else
		ret = rtk_i2c_write(priv->i2c_object, priv->msgv->addr, w_msgv->buffer, w_msgv->length, 1);
#endif
	}
	else if ((w_msgv->flags & I2C_M_READ) != 0) {

		i2cinfo("i2c reading");
#ifdef CONFIG_I2C_SLAVE

		ret = i2c_slave_read(priv->i2c_object, w_msgv->buffer, w_msgv->length);
#else
		ret = rtk_i2c_read(priv->i2c_object, priv->msgv->addr, w_msgv->buffer, w_msgv->length, 1);
#endif
	}

#endif  /* #ifdef CONFIG_I2C_WRITEREAD */

	priv->intstate = INTSTATE_DONE;

	return ret;

}

/************************************************************************************
 * Name: amebad_i2c_isr
 *
 * Description:
 *   Common I2C interrupt service routine
 *
 ************************************************************************************/

#ifndef CONFIG_I2C_POLLED
static int amebad_i2c_isr(int irq, void *context, FAR void *arg)
{
	struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)arg;

	DEBUGASSERT(priv != NULL);
	return amebad_i2c_isr_process(priv);
}
#endif

/************************************************************************************
 * Name: amebad_i2c_init
 *
 * Description:
 *   Setup the I2C hardware, ready for operation with defaults
 *
 ************************************************************************************/

static int amebad_i2c_init(FAR struct amebad_i2c_priv_s *priv)
{
	/* Power-up and configure GPIOs */
	DEBUGASSERT(priv);
	DEBUGASSERT(!priv->i2c_object);
	priv->i2c_object = (i2c_t *)kmm_malloc(sizeof(i2c_t));
	DEBUGASSERT(priv->i2c_object);
	i2c_init(priv->i2c_object, priv->config->sda_pin, priv->config->scl_pin);

#ifdef CONFIG_I2C_WRITEREAD
	i2c_restart_enable(priv->i2c_object);
#endif
#ifdef CONFIG_I2C_SLAVE
	i2c_slave_mode(priv->i2c_object, 1);
#endif
#ifndef CONFIG_I2C_POLLED
	irq_attach(priv->config->irq, amebad_i2c_isr, priv);
	up_enable_irq(priv->config->irq);
#endif

	return OK;
}

/************************************************************************************
 * Name: amebad_i2c_deinit
 *
 * Description:
 *   Shutdown the I2C hardware
 *
 ************************************************************************************/

static int amebad_i2c_deinit(FAR struct amebad_i2c_priv_s *priv)
{

	DEBUGASSERT(priv);
	DEBUGASSERT(priv->i2c_object);

#ifndef CONFIG_I2C_POLLED
	up_disable_irq(priv->config->irq);
	irq_detach(priv->config->irq);
#endif

	kmm_free(priv->i2c_object);
	priv->i2c_object = NULL;

	return OK;
}

/************************************************************************************
 * Device Driver Operations
 ************************************************************************************/
/************************************************************************************
 * Name: amebad_i2c_setfrequency
 *
 * Description:
 *   Set the I2C frequency
 *
 ************************************************************************************/
static uint32_t amebad_i2c_setfrequency(FAR struct i2c_dev_s *dev, uint32_t frequency)
{
	FAR struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)dev;

	amebad_i2c_sem_wait(priv);

	((struct amebad_i2c_priv_s *)dev)->frequency = frequency;
	i2c_frequency(priv->i2c_object, frequency);

	amebad_i2c_sem_post(priv);
	return ((struct amebad_i2c_priv_s *)dev)->frequency;
}

/************************************************************************************
 * Name: amebad_i2c_setaddress
 *
 * Description:
 *   Set the I2C slave address
 *
 ************************************************************************************/

static int amebad_i2c_setaddress(FAR struct i2c_dev_s *dev, int addr, int nbits)
{
	FAR struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)dev;

	amebad_i2c_sem_wait(priv);

	((struct amebad_i2c_priv_s *)dev)->address = addr;
	((struct amebad_i2c_priv_s *)dev)->flags = (nbits == 10) ? I2C_M_TEN : 0;

#ifdef CONFIG_I2C_SLAVE
	i2c_slave_address(priv->i2c_object, priv->i2c_object->i2c_idx, addr, 0xFF);
#endif

	amebad_i2c_sem_post(priv);
	return OK;
}

/************************************************************************************
 * Name: amebad_i2c_process
 *
 * Description:
 *   Process I2C data
 *
 ************************************************************************************/

static int amebad_i2c_process(FAR struct i2c_dev_s *dev, FAR struct i2c_msg_s *msgs, int count)
{
	FAR struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)dev;

	priv->msgv = msgs;
	priv->msgc = count;

	int ret = 0;
	ret = amebad_i2c_sem_waitdone(priv);

	amebad_i2c_sem_post(priv);
	return ret;

}


/************************************************************************************
 * Name: amebad_i2c_write
 *
 * Description:
 *   Write I2C data
 *
 ************************************************************************************/

static int amebad_i2c_write(FAR struct i2c_dev_s *dev, const uint8_t *buffer, int buflen)
{
	FAR struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)dev;

	amebad_i2c_sem_wait(priv);	/* ensure that address or flags don't change meanwhile */

	struct i2c_msg_s msgv = {
		.addr = priv->address,
		.flags = priv->flags,
		.buffer = (uint8_t *)buffer,
		.length = buflen
	};

	return amebad_i2c_process(dev, &msgv, 1);
}

/************************************************************************************
 * Name: amebad_i2c_read
 *
 * Description:
 *   Read I2C data
 *
 ************************************************************************************/

static int amebad_i2c_read(FAR struct i2c_dev_s *dev, uint8_t *buffer, int buflen)
{
        FAR struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)dev;

        amebad_i2c_sem_wait(priv);     /* ensure that address or flags don't change meanwhile */

        struct i2c_msg_s msgv = {
                .addr = priv->address,
                .flags = priv->flags | I2C_M_READ,
                .buffer = (uint8_t *)buffer,
                .length = buflen
        };

        return amebad_i2c_process(dev, &msgv, 1);
}

/************************************************************************************
 * Name: amebad_i2c_writeread
 *
 * Description:
 *  Read then write I2C data
 *
 ************************************************************************************/

#ifdef CONFIG_I2C_WRITEREAD
static int amebad_i2c_writeread(FAR struct i2c_dev_s *dev, const uint8_t *wbuffer, int wbuflen, uint8_t *buffer, int buflen)
{
	FAR struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)dev;

	amebad_i2c_sem_wait(priv);	/* Ensure that address or flags don't change meanwhile */

        struct i2c_msg_s msgv[2] = {
		{
                	.addr = priv->address,
                	.flags = priv->flags,
                	.buffer = (uint8_t *)wbuffer,
                	.length = wbuflen
		},
		{
                	.addr = priv->address,
                	.flags = priv->flags | ((buflen > 0) ? I2C_M_READ : I2C_M_NORESTART),
                	.buffer = (uint8_t *)buffer,
                	.length = (buflen > 0) ? buflen : -buflen
		}
        };

        return amebad_i2c_process(dev, msgv, 2);
}
#endif

#ifdef CONFIG_I2C_TRANSFER
/************************************************************************************
 * Name: amebad_i2c_transfer
 *
 * Description:
 *   Generic I2C transfer function
 *
 ************************************************************************************/

static int amebad_i2c_transfer(FAR struct i2c_dev_s *dev, FAR struct i2c_msg_s *msgs, int count)
{
	FAR struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)dev;

	amebad_i2c_sem_wait(priv);

	return amebad_i2c_process(dev, msgs, count);
}
#endif

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: up_i2cinitialize
 *
 * Description:
 *   Initialize one I2C bus
 *
 ************************************************************************************/

FAR struct i2c_dev_s *up_i2cinitialize(int port)
{
	struct amebad_i2c_priv_s *priv = NULL;
	irqstate_t flags;

	/* Get I2C private structure */

	priv = (struct amebad_i2c_priv_s *)&amebad_i2c1_priv;
	/* Initialize private data for the first time, increment reference count,
	 * power-up hardware and configure GPIOs.
	 */

	flags = irqsave();

	if ((volatile int)priv->refs++ == 0) {
		amebad_i2c_sem_init(priv);
		amebad_i2c_init(priv);
	}

	irqrestore(flags);

	return (struct i2c_dev_s *)priv;
}

/************************************************************************************
 * Name: up_i2cuninitialize
 *
 * Description:
 *   Uninitialize an I2C bus
 *
 ************************************************************************************/

int up_i2cuninitialize(FAR struct i2c_dev_s *dev)
{
	FAR struct amebad_i2c_priv_s *priv = (struct amebad_i2c_priv_s *)dev;
	irqstate_t flags;

	DEBUGASSERT(dev);

	/* Decrement reference count and check for underflow */

	if (priv->refs == 0) {
		return ERROR;
	}

	flags = irqsave();

	if (--priv->refs > 0) {
		irqrestore(flags);
		return OK;
	}

	irqrestore(flags);

	/* Disable power and other HW resource (GPIO's) */

	amebad_i2c_deinit(priv);

	/* Release unused resources */

	amebad_i2c_sem_destroy(priv);
	return OK;
}

/************************************************************************************
 * Name: up_i2creset
 *
 * Description:
 *   Reset an I2C bus
 *
 ************************************************************************************/

#ifdef CONFIG_I2C_RESET
int up_i2creset(FAR struct i2c_dev_s *dev)
{
	FAR struct amebad_i2c_priv_s *priv = (FAR struct amebad_i2c_priv_s *)dev;

	i2c_reset(priv->i2c_object);

	return 0;
}
#endif							/* CONFIG_I2C_RESET */
