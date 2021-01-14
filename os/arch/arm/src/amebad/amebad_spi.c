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
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <semaphore.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/irq.h>
#include <tinyara/arch.h>
#include <tinyara/semaphore.h>
#include <tinyara/spi/spi.h>
#include <tinyara/pm/pm.h>
#include <tinyara/mm/mm.h>

#include <arch/board/board.h>

#include "up_internal.h"
#include "up_arch.h"

#include "chip.h"

#include "amebad_spi.h"
#include "mbed/targets/hal/rtl8721d/PinNames.h"
#include "mbed/hal/spi_api.h"

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/

/* Configuration ********************************************************************/

/* SPI interrupts */

#ifdef CONFIG_AMEBAD_SPI_INTERRUPTS
#error "Interrupt driven SPI not yet supported"
#endif

#if defined(CONFIG_AMEBAD_SPI_DMA)
#error "DMA mode is not yet supported"
#endif

/* Can't have both interrupt driven SPI and SPI DMA */

#if defined(CONFIG_AMEBAD_SPI_INTERRUPTS) && defined(CONFIG_AMEBAD_SPI_DMA)
#error "Cannot enable both interrupt mode and DMA mode for SPI"
#endif

#undef spierr
#undef spiinfo
#if defined(DEBUG_IMX_I2C)
#define spiinfo(format, ...)   printf(format, ##__VA_ARGS__)
#define spierr(format, ...)    printf(format, ##__VA_ARGS__)
#else
#define spiinfo(format, ...)
//#define spierr(format, ...)
#endif
#define spierr(format, ...)   printf(format, ##__VA_ARGS__)

#define AMEBAD_SPI_MASTER	0
#define AMEBAD_SPI_SLAVE	1

/************************************************************************************
 * Private Types
 ************************************************************************************/

struct amebad_spidev_s {
	struct spi_dev_s spidev;    /* Externally visible part of the SPI interface */
//	uint32_t spibase;           /* SPIn base address */
//#ifdef CONFIG_AMEBAD_SPI_INTERRUPTS
//	uint8_t spiirq;             /* SPI IRQ number */
//#endif
	sem_t exclsem;              /* Held while chip is selected for mutual exclusion */
	uint32_t frequency;         /* Requested clock frequency */
	uint32_t actual;            /* Actual clock frequency */

	spi_t spi_object;
	uint32_t spi_idx;
	PinName spi_mosi;
	PinName spi_miso;
	PinName spi_sclk;
	PinName spi_cs;
	int8_t nbits;               /* Width of word in bits */
	uint8_t mode;               /* Mode 0,1,2,3 */
	int role;
};

enum amebad_delay_e {
	SPI_PCS_TO_SCK = 1,       /* PCS-to-SCK delay. */
	SPI_LAST_SCK_TO_PCS,      /* Last SCK edge to PCS delay. */
	SPI_BETWEEN_TRANSFER      /* Delay between transfers. */
};

/************************************************************************************
 * Private Function Prototypes
 ************************************************************************************/

/* Helpers */

static inline uint32_t amebad_spi_getreg32(FAR struct amebad_spidev_s *priv,
					uint8_t offset);
static inline void amebad_spi_putreg32(FAR struct amebad_spidev_s *priv,
					uint8_t offset, uint32_t value);
static inline uint16_t amebad_spi_readword(FAR struct amebad_spidev_s *priv);
static inline void amebad_spi_writeword(FAR struct amebad_spidev_s *priv,
					uint16_t byte);
static inline bool amebad_spi_9to16bitmode(FAR struct amebad_spidev_s *priv);
static inline void amebad_spi_master_set_delays(FAR struct amebad_spidev_s
					*priv, uint32_t delay_ns,
					enum amebad_delay_e type);
static inline void amebad_spi_master_set_delay_scaler(FAR struct
					amebad_spidev_s *priv,
					uint32_t scaler,
					enum amebad_delay_e type);

/* SPI methods */

static int amebad_spi_lock(FAR struct spi_dev_s *dev, bool lock);
static uint32_t amebad_spi_setfrequency(FAR struct spi_dev_s *dev,
					uint32_t frequency);
static void amebad_spi_setmode(FAR struct spi_dev_s *dev,
				enum spi_mode_e mode);
static void amebad_spi_setbits(FAR struct spi_dev_s *dev, int nbits);
#ifdef CONFIG_SPI_HWFEATURES
static int amebad_spi_hwfeatures(FAR struct spi_dev_s *dev,
				amebad_spi_hwfeatures_t features);
#endif
static uint16_t amebad_spi_send(FAR struct spi_dev_s *dev, uint16_t wd);
static void amebad_spi_exchange(FAR struct spi_dev_s *dev,
				FAR const void *txbuffer, FAR void *rxbuffer,
				size_t nwords);
#ifndef CONFIG_SPI_EXCHANGE
static void amebad_spi_sndblock(FAR struct spi_dev_s *dev,
				FAR const void *txbuffer, size_t nwords);
static void amebad_spi_recvblock(FAR struct spi_dev_s *dev, FAR void *rxbuffer,
				size_t nwords);
#endif

/* Initialization */

static void amebad_spi_bus_initialize(FAR struct amebad_spidev_s *priv);

/************************************************************************************
 * Private Data
 ************************************************************************************/

static const struct spi_ops_s g_spi0ops = {
	.lock         = amebad_spi_lock,
	.select       = amebad_spi0select,
	.setfrequency = amebad_spi_setfrequency,
	.setmode      = amebad_spi_setmode,
	.setbits      = amebad_spi_setbits,
#ifdef CONFIG_SPI_HWFEATURES
	.hwfeatures   = amebad_spi_hwfeatures,
#endif
	.status       = amebad_spi0status,
#ifdef CONFIG_SPI_CMDDATA
	.cmddata      = amebad_spi0cmddata,
#endif
	.send         = amebad_spi_send,
#ifdef CONFIG_SPI_EXCHANGE
	.exchange     = amebad_spi_exchange,
#else
	.sndblock     = amebad_spi_sndblock,
	.recvblock    = amebad_spi_recvblock,
#endif
#ifdef CONFIG_SPI_CALLBACK
	.registercallback = amebad_spi0register,  /* Provided externally */
#else
	.registercallback = 0,                     /* Not implemented */
#endif
};

static struct amebad_spidev_s g_spi0dev = {
	.spidev       = {
		&g_spi0ops
	},
	/*
	.spibase      = AMEBAD_SPI1_BASE,
#ifdef CONFIG_AMEBAD_SPI_INTERRUPTS
	.spiirq       = AMEBAD_IRQ_SPI1,
#endif
#ifdef CONFIG_AMEBAD_SPI_DMA
	.rxch         = DMAMAP_SPI1_RX,
	.txch         = DMAMAP_SPI1_TX,
#endif
*/

	.spi_object = {0},

	.spi_idx = MBED_SPI0,
	.spi_mosi = PA_16,
	.spi_miso = PA_17,
	.spi_sclk = PA_18,
	.spi_cs = PA_19,
	.nbits = 8,
	.mode = SPIDEV_MODE0,
	.role = AMEBAD_SPI_SLAVE,
};

static const struct spi_ops_s g_spi1ops = {
	.lock         = amebad_spi_lock,
	.select       = amebad_spi1select,
	.setfrequency = amebad_spi_setfrequency,
	.setmode      = amebad_spi_setmode,
	.setbits      = amebad_spi_setbits,
#ifdef CONFIG_SPI_HWFEATURES
	.hwfeatures   = amebad_spi_hwfeatures,
#endif
	.status       = amebad_spi1status,
#ifdef CONFIG_SPI_CMDDATA
	.cmddata      = amebad_spi1cmddata,
#endif
	.send         = amebad_spi_send,
#ifdef CONFIG_SPI_EXCHANGE
	.exchange     = amebad_spi_exchange,
#else
	.sndblock     = amebad_spi_sndblock,
	.recvblock    = amebad_spi_recvblock,
#endif
#ifdef CONFIG_SPI_CALLBACK
	.registercallback = amebad_spi1register,  /* Provided externally */
#else
	.registercallback = 0,                     /* Not implemented */
#endif
};

static struct amebad_spidev_s g_spi1dev = {
	.spidev       = {
		&g_spi1ops
	},
	/*
	.spibase      = AMEBAD_SPI1_BASE,
#ifdef CONFIG_AMEBAD_SPI_INTERRUPTS
	.spiirq       = AMEBAD_IRQ_SPI1,
#endif
#ifdef CONFIG_AMEBAD_SPI_DMA
	.rxch         = DMAMAP_SPI1_RX,
	.txch         = DMAMAP_SPI1_TX,
#endif
*/

	.spi_object = {0},

	.spi_idx = MBED_SPI1,
	.spi_mosi = PB_4,
	.spi_miso = PB_5,
	.spi_sclk = PB_6,
	.spi_cs = PB_7,
	.nbits = 8,
	.mode = SPIDEV_MODE0,
	.role = AMEBAD_SPI_MASTER
};
/************************************************************************************
 * Private Functions
 ************************************************************************************/

/************************************************************************************
 * Name: amebad_spi_getreg8
 *
 * Description:
 *   Get the contents of the SPI register at offset
 *
 * Input Parameters:
 *   priv   - private SPI device structure
 *   offset - offset to the register of interest
 *
 * Returned Value:
 *   The contents of the 8-bit register
 *
 ************************************************************************************/

static inline uint8_t amebad_spi_getreg8(FAR struct amebad_spidev_s *priv,
					uint8_t offset)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_spi_putreg8
 *
 * Description:
 *   Write a 8-bit value to the SPI register at offset
 *
 * Input Parameters:
 *   priv   - private SPI device structure
 *   offset - offset to the register of interest
 *   value  - the 8-bit value to be written
 *
 ************************************************************************************/

static inline void amebad_spi_putreg8(FAR struct amebad_spidev_s *priv,
				uint8_t offset, uint8_t value)
{
	return;
}

/************************************************************************************
 * Name: amebad_spi_getreg
 *
 * Description:
 *   Get the contents of the SPI register at offset
 *
 * Input Parameters:
 *   priv   - private SPI device structure
 *   offset - offset to the register of interest
 *
 * Returned Value:
 *   The contents of the 32-bit register
 *
 ************************************************************************************/

static inline uint32_t amebad_spi_getreg32(FAR struct amebad_spidev_s *priv,
					uint8_t offset)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_spi_putreg
 *
 * Description:
 *   Write a 16-bit value to the SPI register at offset
 *
 * Input Parameters:
 *   priv   - private SPI device structure
 *   offset - offset to the register of interest
 *   value  - the 32-bit value to be written
 *
 * Returned Value:
 *   The contents of the 32-bit register
 *
 ************************************************************************************/

static inline void amebad_spi_putreg32(FAR struct amebad_spidev_s *priv,
					uint8_t offset, uint32_t value)
{
	return;
}

/************************************************************************************
 * Name: amebad_spi_readword
 *
 * Description:
 *   Read one word from SPI
 *
 * Input Parameters:
 *   priv - Device-specific state data
 *
 * Returned Value:
 *   word as read
 *
 ************************************************************************************/

static inline uint16_t amebad_spi_readword(FAR struct amebad_spidev_s *priv)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_spi_writeword
 *
 * Description:
 *   Write one word to SPI
 *
 * Input Parameters:
 *   priv - Device-specific state data
 *   word - word to send
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static inline void amebad_spi_writeword(FAR struct amebad_spidev_s *priv, uint16_t word)
{
	return;
}

/************************************************************************************
 * Name: amebad_spi_readbyte
 *
 * Description:
 *   Read one byte from SPI
 *
 * Input Parameters:
 *   priv - Device-specific state data
 *
 * Returned Value:
 *   Byte as read
 *
 ************************************************************************************/

static inline uint8_t amebad_spi_readbyte(FAR struct amebad_spidev_s *priv)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_spi_writebyte
 *
 * Description:
 *   Write one 8-bit frame to the SPI FIFO
 *
 * Input Parameters:
 *   priv - Device-specific state data
 *   byte - Byte to send
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static inline void amebad_spi_writebyte(FAR struct amebad_spidev_s *priv, uint8_t byte)
{
	return;
}

/************************************************************************************
 * Name: amebad_spi_9to16bitmode
 *
 * Description:
 *   Check if the SPI is operating in more then 8 bit mode
 *
 * Input Parameters:
 *   priv     - Device-specific state data
 *
 * Returned Value:
 *   true: >8 bit mode-bit mode, false: <= 8-bit mode
 *
 ************************************************************************************/

static inline bool amebad_spi_9to16bitmode(FAR struct amebad_spidev_s *priv)
{
	bool ret;

	if (priv->nbits < 9) ret = false;
	else ret = true;

	return ret;
}

/************************************************************************************
 * Name: amebad_spi_modifyreg
 *
 * Description:
 *   Clear and set bits in register
 *
 * Input Parameters:
 *   priv    - Device-specific state data
 *   offset  - Register offset
 *   clrbits - The bits to clear
 *   setbits - The bits to set
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static void amebad_spi_modifyreg32(FAR struct amebad_spidev_s *priv,
				uint8_t offset, uint32_t clrbits,
				uint32_t setbits)
{
}

/************************************************************************************
 * Name: amebad_spi_master_set_delays
 *
 * Description:
 *   SET SPI Delay times
 *
 * Input Parameters:
 *   priv   - Device-specific state data
 *   scaler - scaler value
 *   type   - delay time type
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static inline void amebad_spi_master_set_delay_scaler(FAR struct amebad_spidev_s *priv,
						uint32_t scaler,
						enum amebad_delay_e type)
{
	/*
	switch (type) {
	case SPI_PCS_TO_SCK:
		amebad_spi_modifyreg32(priv, AMEBAD_SPI_CCR_OFFSET,
					SPI_CCR_PCSSCK_MASK, 0);
		amebad_spi_modifyreg32(priv, AMEBAD_SPI_CCR_OFFSET, 0,
					SPI_CCR_PCSSCK(scaler));
	break;

	case SPI_LAST_SCK_TO_PCS:
		amebad_spi_modifyreg32(priv, AMEBAD_SPI_CCR_OFFSET,
					SPI_CCR_SCKPCS_MASK, 0);
		amebad_spi_modifyreg32(priv, AMEBAD_SPI_CCR_OFFSET, 0,
					SPI_CCR_SCKPCS(scaler));
	break;

	case SPI_BETWEEN_TRANSFER:
		amebad_spi_modifyreg32(priv, AMEBAD_SPI_CCR_OFFSET, SPI_CCR_DBT_MASK,
					0);
		amebad_spi_modifyreg32(priv, AMEBAD_SPI_CCR_OFFSET, 0,
					SPI_CCR_DBT(scaler));
	break;
	}
	*/
}

/************************************************************************************
 * Name: amebad_spi_master_set_delays
 *
 * Description:
 *   SET SPI Delay times
 *
 * Input Parameters:
 *   priv     - Device-specific state data
 *   delay_ns - delay time in nano seconds
 *   type     - delay time type
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/
static inline void amebad_spi_master_set_delays(FAR struct amebad_spidev_s *priv,
						uint32_t delay_ns,
						enum amebad_delay_e type)
{
}

/************************************************************************************
 * Name: amebad_spi_lock
 *
 * Description:
 *   On SPI busses where there are multiple devices, it will be necessary to
 *   lock SPI to have exclusive access to the busses for a sequence of
 *   transfers.  The bus should be locked before the chip is selected. After
 *   locking the SPI bus, the caller should then also call the setfrequency,
 *   setbits, and setmode methods to make sure that the SPI is properly
 *   configured for the device.  If the SPI buss is being shared, then it
 *   may have been left in an incompatible state.
 *
 * Input Parameters:
 *   dev  - Device-specific state data
 *   lock - true: Lock spi bus, false: unlock SPI bus
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static int amebad_spi_lock(FAR struct spi_dev_s *dev, bool lock)
{
	FAR struct amebad_spidev_s *priv = (FAR struct amebad_spidev_s *)dev;
	int ret;

	if (lock) {
		/* Take the semaphore (perhaps waiting) */

		do {
			ret = sem_wait(&priv->exclsem);

			/* The only case that an error should occur here is if the wait was
			 * awakened by a signal.
			 */

			DEBUGASSERT(ret == OK || errno == EINTR);
		} while (errno == EINTR);
	} else {
		(void)sem_post(&priv->exclsem);
		ret = OK;
	}

	return ret;
}

/************************************************************************************
 * Name: amebad_spi0select
 *
 * Description:
 *   Enable/disable the SPI slave select. The implementation of this method
 *   must include handshaking:  If a device is selected, it must hold off
 *   all other attempts to select the device until the device is deselecte.
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   devid -    Device Id
 *   selected - whether it is selected or not
 *
 * Returned Value: None
 *  
 *
 ************************************************************************************/
void amebad_spi0select(FAR struct spi_dev_s *dev, enum spi_dev_e devid, bool selected)
{
	return;
}

/************************************************************************************
 * Name: amebad_spi0status
 *
 * Description:
 *   Get the spi status
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   devid  - Device ID
 *
 * Returned Value:
 *   Returns the SPI status
 *
 ************************************************************************************/
uint8_t amebad_spi0status(FAR struct spi_dev_s *dev, enum spi_dev_e devid)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_spi0cmddata
 *
 * Description:
 *   Send cmd to device according to devid.
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   devid -   Device ID
 *   cmd - the command that need to be sent
 *
 * Returned Value:
 *   TBD
 *
 ************************************************************************************/
int amebad_spi0cmddata(FAR struct spi_dev_s *dev, uint32_t devid, bool cmd)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_spi1select
 *
 * Description:
 *   Enable/disable the SPI slave select. The implementation of this method
 *   must include handshaking:  If a device is selected, it must hold off
 *   all other attempts to select the device until the device is deselecte.
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   devid -    Device Id
 *   selected - whether it is selected or not
 *
 * Returned Value: None
 *  
 *
 ************************************************************************************/
void amebad_spi1select(FAR struct spi_dev_s *dev, enum spi_dev_e devid, bool selected)
{
	return;
}

/************************************************************************************
 * Name: amebad_spi1status
 *
 * Description:
 *   Get the spi status
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   devid  - Device ID
 *
 * Returned Value:
 *   Returns the SPI status
 *
 ************************************************************************************/
uint8_t amebad_spi1status(FAR struct spi_dev_s *dev, enum spi_dev_e devid)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_spi1cmddata
 *
 * Description:
 *   Send cmd to device according to devid.
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   devid -   Device ID
 *   cmd - the command that need to be sent
 *
 * Returned Value:
 *   TBD
 *
 ************************************************************************************/
int amebad_spi1cmddata(FAR struct spi_dev_s *dev, uint32_t devid, bool cmd)
{
	return 0;
}

/************************************************************************************
 * Name: amebad_spi_setfrequency
 *
 * Description:
 *   Set the SPI frequency.
 *
 * Input Parameters:
 *   dev -       Device-specific state data
 *   frequency - The SPI frequency requested
 *
 * Returned Value:
 *   Returns the actual frequency selected
 *
 ************************************************************************************/

static uint32_t amebad_spi_setfrequency(FAR struct spi_dev_s *dev,
					uint32_t frequency)
{
	FAR struct amebad_spidev_s *priv = (FAR struct amebad_spidev_s *)dev;

	/* Has the SPI bus frequency changed? */

	if (frequency != priv->frequency) {
		/* Disable SPI if it is enabled */

		priv->frequency = frequency;

		if (priv->role == AMEBAD_SPI_MASTER)
			spi_frequency(&priv->spi_object, priv->frequency);
	}

	return priv->frequency;
}

/************************************************************************************
 * Name: amebad_spi_setmode
 *
 * Description:
 *   Set the SPI mode.  see enum spi_mode_e mode for mode definitions
 *
 * Input Parameters:
 *   dev  - Device-specific state data
 *   mode - The SPI mode requested
 *
 * Returned Value:
 *   Returns the actual frequency selected
 *
 ************************************************************************************/

static void amebad_spi_setmode(FAR struct spi_dev_s *dev, enum spi_mode_e mode)
{
	FAR struct amebad_spidev_s *priv = (FAR struct amebad_spidev_s *)dev;

	spiinfo("mode=%d\n", mode);

	/* Has the mode changed? */

	if (mode != priv->mode) {
		/* Disable SPI if it is enabled */

		priv->mode = mode;
		spi_format(&priv->spi_object, priv->nbits, priv->mode, priv->role);

	}
}

/************************************************************************************
 * Name: amebad_spi_setbits
 *
 * Description:
 *   Set the number of bits per word.
 *
 * Input Parameters:
 *   dev   - Device-specific state data
 *   nbits - The number of bits requested
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static void amebad_spi_setbits(FAR struct spi_dev_s *dev, int nbits)
{
	FAR struct amebad_spidev_s *priv = (FAR struct amebad_spidev_s *)dev;

	spiinfo("nbits=%d\n", nbits);

	/* Has the number of bits changed? */

	if (nbits != priv->nbits) {

		if (nbits < 4 || nbits > 16) {
			return;
		}

		/* Save the selection so the subsequence re-configurations will be faster */

		priv->nbits = nbits;
		spi_format(&priv->spi_object, priv->nbits, priv->mode, priv->role);

	}
}

/****************************************************************************
 * Name: amebad_spi_hwfeatures
 *
 * Description:
 *   Set hardware-specific feature flags.
 *
 * Input Parameters:
 *   dev      - Device-specific state data
 *   features - H/W feature flags
 *
 * Returned Value:
 *   Zero (OK) if the selected H/W features are enabled; A negated errno
 *   value if any H/W feature is not supportable.
 *
 ****************************************************************************/

#ifdef CONFIG_SPI_HWFEATURES
static int amebad_spi_hwfeatures(FAR struct spi_dev_s *dev,
				amebad_spi_hwfeatures_t features)
{
#ifdef CONFIG_SPI_BITORDER
	FAR struct amebad_spidev_s *priv = (FAR struct amebad_spidev_s *)dev;

	spiinfo("features=%08x\n", features);

	/* Other H/W features are not supported */

	return -ENOSYS;
#endif
	return -ENOSYS;
}
#endif

/************************************************************************************
 * Name: amebad_spi_send
 *
 * Description:
 *   Exchange one word on SPI
 *
 * Input Parameters:
 *   dev - Device-specific state data
 *   wd  - The word to send.  the size of the data is determined by the
 *         number of bits selected for the SPI interface.
 *
 * Returned Value:
 *   response
 *
 ************************************************************************************/

static uint16_t amebad_spi_send(FAR struct spi_dev_s *dev, uint16_t wd)
{
	FAR struct amebad_spidev_s *priv = (FAR struct amebad_spidev_s *)dev;

	uint16_t ret;
	DEBUGASSERT(priv);

	if (priv->role == AMEBAD_SPI_MASTER) {

		ret = spi_master_write(&priv->spi_object, wd);
	} else if (priv->role == AMEBAD_SPI_SLAVE) {

		spi_slave_write(&priv->spi_object, wd);
		ret = wd;
	}

	/* Check and clear any error flags (Reading from the SR clears the error
	 * flags).
	 */

	spiinfo("Sent: %04x Return: %04x\n", wd, ret);

	return ret;
}

/************************************************************************************
 * Name: amebad_spi_exchange (no DMA).  aka amebad_spi_exchange_nodma
 *
 * Description:
 *   Exchange a block of data on SPI without using DMA
 *
 * Input Parameters:
 *   dev      - Device-specific state data
 *   txbuffer - A pointer to the buffer of data to be sent
 *   rxbuffer - A pointer to a buffer in which to receive data
 *   nwords   - the length of data to be exchaned in units of words.
 *              The wordsize is determined by the number of bits-per-word
 *              selected for the SPI interface.  If nbits <= 8, the data is
 *              packed into uint8_t's; if nbits >8, the data is packed into uint16_t's
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

#if !defined(CONFIG_AMEBAD_SPI_DMA) || defined(CONFIG_AMEBAD_DMACAPABLE)
#if !defined(CONFIG_AMEBAD_SPI_DMA)
static void amebad_spi_exchange(FAR struct spi_dev_s *dev,
				FAR const void *txbuffer, FAR void *rxbuffer,
				size_t nwords)
#else
static void amebad_spi_exchange_nodma(FAR struct spi_dev_s *dev,
				FAR const void *txbuffer,
				FAR void *rxbuffer, size_t nwords)
#endif
{
	FAR struct amebad_spidev_s *priv = (FAR struct amebad_spidev_s *)dev;
	DEBUGASSERT(priv);

	spiinfo("txbuffer=%p rxbuffer=%p nwords=%d\n", txbuffer, rxbuffer, nwords);

	/* 8- or 16-bit mode? */

	if (amebad_spi_9to16bitmode(priv)) {
		/* 16-bit mode */

		const uint16_t *src = (const uint16_t *)txbuffer;
		uint16_t *dest = (uint16_t *)rxbuffer;
		uint16_t word;

		while (nwords-- > 0) {
			/* Get the next word to write.  Is there a source buffer? */

			if (src) {
				word = *src++;
			} else {
				word = 0xffff;
			}

			/* Exchange one word */

			word = amebad_spi_send(dev, word);

			/* Is there a buffer to receive the return value? */

			if (dest) {
				*dest++ = word;
			}
		}
	} else {
		/* 8-bit mode */

		const uint8_t *src = (const uint8_t *)txbuffer;
		uint8_t *dest = (uint8_t *)rxbuffer;
		uint8_t word;

		while (nwords-- > 0) {
			/* Get the next word to write.  Is there a source buffer? */

			if (src) {
				word = *src++;
			} else {
				word = 0xff;
			}

			/* Exchange one word */

			word = (uint8_t)amebad_spi_send(dev, (uint16_t) word);

			/* Is there a buffer to receive the return value? */

			if (dest) {
				*dest++ = word;
			}
		}
	}
}
#endif /* !CONFIG_AMEBAD_SPI_DMA || CONFIG_AMEBAD_DMACAPABLE */

/****************************************************************************
 * Name: amebad_spi_sndblock
 *
 * Description:
 *   Send a block of data on SPI
 *
 * Input Parameters:
 *   dev      - Device-specific state data
 *   txbuffer - A pointer to the buffer of data to be sent
 *   nwords   - the length of data to send from the buffer in number of words.
 *              The wordsize is determined by the number of bits-per-word
 *              selected for the SPI interface.  If nbits <= 8, the data is
 *              packed into uint8_t's; if nbits >8, the data is packed into uint16_t's
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

#ifndef CONFIG_SPI_EXCHANGE
static void amebad_spi_sndblock(FAR struct spi_dev_s *dev,
				FAR const void *txbuffer, size_t nwords)
{
	spiinfo("txbuffer=%p nwords=%d\n", txbuffer, nwords);
	return amebad_spi_exchange(dev, txbuffer, NULL, nwords);
}
#endif

/************************************************************************************
 * Name: amebad_spi_recvblock
 *
 * Description:
 *   Receive a block of data from SPI
 *
 * Input Parameters:
 *   dev      - Device-specific state data
 *   rxbuffer - A pointer to the buffer in which to recieve data
 *   nwords   - the length of data that can be received in the buffer in number
 *              of words.  The wordsize is determined by the number of bits-per-word
 *              selected for the SPI interface.  If nbits <= 8, the data is
 *              packed into uint8_t's; if nbits >8, the data is packed into uint16_t's
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

#ifndef CONFIG_SPI_EXCHANGE
static void amebad_spi_recvblock(FAR struct spi_dev_s *dev, FAR void *rxbuffer,
				size_t nwords)
{
	spiinfo("rxbuffer=%p nwords=%d\n", rxbuffer, nwords);
	return amebad_spi_exchange(dev, NULL, rxbuffer, nwords);
}
#endif

/************************************************************************************
 * Name: amebad_spi_clock_enable
 *
 * Description:
 *   Ungate SPI clock
 *
 ************************************************************************************/

void amebad_spi_clock_enable(uint32_t base)
{
	return;
}

/************************************************************************************
 * Name: amebad_spi_clock_disable
 *
 * Description:
 *   Gate SPI clock
 *
 ************************************************************************************/

void amebad_spi_clock_disable(uint32_t base)
{
	return;
}

/************************************************************************************
 * Name: amebad_spi_bus_initialize
 *
 * Description:
 *   Initialize the selected SPI bus in its default state (Master, 8-bit, mode 0, etc.)
 *
 * Input Parameters:
 *   priv   - private SPI device structure
 *
 * Returned Value:
 *   None
 *
 ************************************************************************************/

static void amebad_spi_bus_initialize(struct amebad_spidev_s *priv)
{

	DEBUGASSERT(priv);
	DEBUGASSERT(&priv->spi_object);

	priv->spi_object.spi_idx = priv->spi_idx;
	spi_init(&priv->spi_object, priv->spi_mosi, priv->spi_miso, priv->spi_sclk, priv->spi_cs);

	spi_format(&priv->spi_object, priv->nbits, priv->mode, priv->role);

}

/************************************************************************************
 * Public Functions
 ************************************************************************************/

/************************************************************************************
 * Name: amebad_spibus_initialize
 *
 * Description:
 *   Initialize the selected SPI bus
 *
 * Input Parameters:
 *   Port number (for hardware that has mutiple SPI interfaces)
 *
 * Returned Value:
 *   Valid SPI device structure reference on success; a NULL on failure
 *
 ************************************************************************************/

FAR struct spi_dev_s *amebad_spibus_initialize(int bus)
{
	FAR struct amebad_spidev_s *priv = NULL;

	irqstate_t flags = irqsave();

	if (bus == 1) {
		/* Select SPI1 */

		priv = &g_spi0dev;

		/* Only configure if the bus is not already configured */

		amebad_spi_bus_initialize(priv);

	} else if (bus == 2) {
		/* Select SPI2 */

		priv = &g_spi1dev;

		/* Only configure if the bus is not already configured */

		amebad_spi_bus_initialize(priv);

	} else {
		spierr("ERROR: Unsupported SPI bus: %d\n", bus);
		return NULL;
	}

	irqrestore(flags);

	return (FAR struct spi_dev_s *)priv;
}

/************************************************************************************
 * Name: up_spiinitialize
 *
 * Description:
 *   Initialize the selected SPI bus
 *
 * Input Parameters:
 *   Port number (for hardware that has mutiple SPI interfaces)
 *
 * Returned Value:
 *   Valid SPI device structure reference on success; a NULL on failure
 *
 ************************************************************************************/

FAR struct spi_dev_s *up_spiinitialize(int port)
{
	FAR struct amebad_spidev_s *priv = NULL;

	irqstate_t flags = irqsave();

	if (port == 0) {
		/* Select SPI1 */

		priv = &g_spi0dev;

		/* Only configure if the bus is not already configured */

		amebad_spi_bus_initialize(priv);
	} 
	else if (port == 1) {
		/* Select SPI1 */

		priv = &g_spi1dev;

		/* Only configure if the bus is not already configured */

		amebad_spi_bus_initialize(priv);
	} else
	{
		spierr("ERROR: Unsupported SPI bus: %d\n", port);
		return NULL;
	}

	irqrestore(flags);

	return (FAR struct spi_dev_s *)priv;
}
