/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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
 * os/arch/arm/src/s5j/chip/s5jt200_pinconfig.h
 *
 *   Copyright (C) 2017 Gregory Nutt. All rights reserved.
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

#ifndef __ARCH_ARM_SRC_S5J_CHIP_S5JT200_PINCONFIG_H__
#define __ARCH_ARM_SRC_S5J_CHIP_S5JT200_PINCONFIG_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GPIO pin definitions *****************************************************/

/* SPI FLASH */
#define GPIO_SFLASH_CLK    (GPIO_ALT1 | GPIO_PULLUP | GPIO_FAST3X | GPIO_PORTP1 | GPIO_PIN0)
#define GPIO_SFLASH_CS     (GPIO_ALT1 | GPIO_PULLUP | GPIO_FAST3X | GPIO_PORTP1 | GPIO_PIN1)
#define GPIO_SFLASH_SI     (GPIO_ALT1 | GPIO_PULLUP | GPIO_FAST3X | GPIO_PORTP1 | GPIO_PIN2)
#define GPIO_SFLASH_SO     (GPIO_ALT1 | GPIO_PULLUP | GPIO_FAST3X | GPIO_PORTP1 | GPIO_PIN3)
#define GPIO_SFLASH_WP     (GPIO_ALT1 | GPIO_PULLUP | GPIO_FAST3X | GPIO_PORTP1 | GPIO_PIN4)
#define GPIO_SFLASH_HLD    (GPIO_ALT1 | GPIO_PULLUP | GPIO_FAST3X | GPIO_PORTP1 | GPIO_PIN5)

/* UART0 */
#define GPIO_UART0_RXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTA2 | GPIO_PIN0)
#define GPIO_UART0_TXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTA2 | GPIO_PIN1)

/* UART1 */
#define GPIO_UART1_RXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN4)
#define GPIO_UART1_TXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN5)

/* UART2 */
#define GPIO_UART2_CTS     (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN4)
#define GPIO_UART2_RTS     (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN5)
#define GPIO_UART2_RXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN6)
#define GPIO_UART2_TXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN7)

/* UART3 */
#define GPIO_UART3_CTS     (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN0)
#define GPIO_UART3_RTS     (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN1)
#define GPIO_UART3_RXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP1 | GPIO_PIN6)
#define GPIO_UART3_TXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP1 | GPIO_PIN7)

/* UART4 */
#define GPIO_UART4_RXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTA3 | GPIO_PIN0)
#define GPIO_UART4_TXD     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTA3 | GPIO_PIN1)

/* PWM */
#define GPIO_PWM_TOUT0     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN0)
#define GPIO_PWM_TOUT1     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN1)
#define GPIO_PWM_TOUT2     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN2)
#define GPIO_PWM_TOUT3     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN3)
#define GPIO_PWM_TOUT4     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN4)
#define GPIO_PWM_TOUT5     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN5)
#define GPIO_PWM_TOUT6     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN6)

/* TICK COUNTER */
#define GPIO_TICKCNT0      (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP2 | GPIO_PIN1)

/* SDIO */
#define GPIO_SDIO_CLK      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP3 | GPIO_PIN0)
#define GPIO_SDIO_CMD      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP3 | GPIO_PIN1)
#define GPIO_SDIO_DATA0    (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP3 | GPIO_PIN2)
#define GPIO_SDIO_DATA1    (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP3 | GPIO_PIN3)
#define GPIO_SDIO_DATA2    (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP3 | GPIO_PIN4)
#define GPIO_SDIO_DATA3    (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP3 | GPIO_PIN5)

/* I2C0 */
#define GPIO_I2C0_SCL      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTA1 | GPIO_PIN0)
#define GPIO_I2C0_SDA      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTA1 | GPIO_PIN1)

/* I2C1 */
#define GPIO_I2C1_SCL      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTA1 | GPIO_PIN2)
#define GPIO_I2C1_SDA      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTA1 | GPIO_PIN3)

/* I2C2 */
#define GPIO_I2C2_SCL      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN0)
#define GPIO_I2C2_SDA      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN1)

/* I2C3 */
#define GPIO_I2C3_SCL      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN2)
#define GPIO_I2C3_SDA      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN3)

/* SPI0 */
#define GPIO_SPI0_CLK      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN0)
#define GPIO_SPI0_CS       (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN1)
#define GPIO_SPI0_MISO     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN2)
#define GPIO_SPI0_MOSI     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST3X | GPIO_PORTP0 | GPIO_PIN3)

/* SPI1 */
#define GPIO_SPI1_CLK      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP4 | GPIO_PIN0)
#define GPIO_SPI1_CS       (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP4 | GPIO_PIN1)
#define GPIO_SPI1_MISO     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP4 | GPIO_PIN2)
#define GPIO_SPI1_MOSI     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTP4 | GPIO_PIN3)

/* SPI2 */
#define GPIO_SPI2_CLK      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN4)
#define GPIO_SPI2_CS       (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN5)
#define GPIO_SPI2_MISO     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN6)
#define GPIO_SPI2_MOSI     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN7)

/* SPI3 */
#define GPIO_SPI3_CLK      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN0)
#define GPIO_SPI3_CS       (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN1)
#define GPIO_SPI3_MISO     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN2)
#define GPIO_SPI3_MOSI     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN3)

/* I2S0 */
#define GPIO_I2S0_BCLK     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG3 | GPIO_PIN0)
#define GPIO_I2S0_LRCK     (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG3 | GPIO_PIN1)
#define GPIO_I2S0_SDO      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG3 | GPIO_PIN2)
#define GPIO_I2S0_SDI      (GPIO_ALT1 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG3 | GPIO_PIN3)

/* MCT */
#define GPIO_MCT0_INTLEV   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG3 | GPIO_PIN0)
#define GPIO_MCT0_TICK     (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG3 | GPIO_PIN1)

/* WLBT DEBUG */
#define GPIO_WLBT_DBG0     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN1)
#define GPIO_WLBT_DBG1     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN2)
#define GPIO_WLBT_DBG2     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN3)
#define GPIO_WLBT_DBG3     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN4)
#define GPIO_WLBT_DBG4     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN5)
#define GPIO_WLBT_DBG5     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN6)
#define GPIO_WBLT_DBG6     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN7)
#define GPIO_WBLT_DBG7     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN0)
#define GPIO_WBLT_DBG8     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN1)
#define GPIO_WBLT_DBG9     (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN2)
#define GPIO_WBLT_DBG10    (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN3)
#define GPIO_WBLT_DBG11    (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN4)
#define GPIO_WBLT_DBG12    (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN5)
#define GPIO_WBLT_DBG13    (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN6)
#define GPIO_WBLT_DBG14    (GPIO_ALT5 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN7)

/* WB2AP */
#define GPIO_WB2AP_CLKOUT  (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN0)
#define GPIO_WB2AP_DOUT0   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN1)
#define GPIO_WB2AP_DOUT1   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN2)
#define GPIO_WB2AP_DOUT2   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN3)
#define GPIO_WB2AP_DOUT3   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN4)
#define GPIO_WB2AP_DOUT4   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN5)
#define GPIO_WB2AP_DOUT5   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN6)
#define GPIO_WB2AP_DOUT6   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN7)
#define GPIO_WB2AP_DOUT7   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN0)
#define GPIO_WB2AP_DOUT8   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN1)
#define GPIO_WB2AP_DOUT9   (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN2)
#define GPIO_WB2AP_DOUT10  (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN3)
#define GPIO_WB2AP_DOUT11  (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN4)
#define GPIO_WB2AP_DOUT12  (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN5)
#define GPIO_WB2AP_DOUT13  (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN6)
#define GPIO_WB2AP_DOUT14  (GPIO_ALT4 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG2 | GPIO_PIN7)

/* ALV */
#define GPIO_ALV_DBG0      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN0)
#define GPIO_ALV_DBG1      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN1)
#define GPIO_ALV_DBG2      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN2)
#define GPIO_ALV_DBG3      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN3)
#define GPIO_ALV_DBG4      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN4)
#define GPIO_ALV_DBG5      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN5)
#define GPIO_ALV_DBG6      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN6)
#define GPIO_ALV_DBG7      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG0 | GPIO_PIN7)
#define GPIO_ALV_DBG8      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN0)
#define GPIO_ALV_DBG9      (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN1)
#define GPIO_ALV_DBG10     (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN2)
#define GPIO_ALV_DBG11     (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN3)
#define GPIO_ALV_DBG12     (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN4)
#define GPIO_ALV_DBG13     (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN5)
#define GPIO_ALV_DBG14     (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN6)
#define GPIO_ALV_DBG15     (GPIO_ALT3 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN7)

/* SFLASH MONITOR */
#define GPIO_SFLASH_MON0   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN0)
#define GPIO_SFLASH_MON1   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN1)
#define GPIO_SFLASH_MON2   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN2)
#define GPIO_SFLASH_MON3   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN3)
#define GPIO_SFLASH_MON4   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN4)
#define GPIO_SFLASH_MON5   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN5)
#define GPIO_SFLASH_MON6   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN6)
#define GPIO_SFLASH_MON7   (GPIO_ALT2 | GPIO_FLOAT | GPIO_FAST1X | GPIO_PORTG1 | GPIO_PIN7)

/* EINT */
#define GPIO_WEXT_INTG10   (GPIO_EINT | GPIO_PORTG1 | GPIO_PIN0)
#define GPIO_WEXT_INTG11   (GPIO_EINT | GPIO_PORTG1 | GPIO_PIN1)
#define GPIO_WEXT_INTG12   (GPIO_EINT | GPIO_PORTG1 | GPIO_PIN2)
#define GPIO_WEXT_INTG13   (GPIO_EINT | GPIO_PORTG1 | GPIO_PIN3)
#define GPIO_WEXT_INTG14   (GPIO_EINT | GPIO_PORTG1 | GPIO_PIN4)
#define GPIO_WEXT_INTG15   (GPIO_EINT | GPIO_PORTG1 | GPIO_PIN5)
#define GPIO_WEXT_INTG16   (GPIO_EINT | GPIO_PORTG1 | GPIO_PIN6)
#define GPIO_WEXT_INTG17   (GPIO_EINT | GPIO_PORTG1 | GPIO_PIN7)
#define GPIO_WEXT_INTG20   (GPIO_EINT | GPIO_PORTG2 | GPIO_PIN0)
#define GPIO_WEXT_INTG21   (GPIO_EINT | GPIO_PORTG2 | GPIO_PIN1)
#define GPIO_WEXT_INTG22   (GPIO_EINT | GPIO_PORTG2 | GPIO_PIN2)
#define GPIO_WEXT_INTG23   (GPIO_EINT | GPIO_PORTG2 | GPIO_PIN3)
#define GPIO_WEXT_INTG24   (GPIO_EINT | GPIO_PORTG2 | GPIO_PIN4)
#define GPIO_WEXT_INTG25   (GPIO_EINT | GPIO_PORTG2 | GPIO_PIN5)
#define GPIO_WEXT_INTG26   (GPIO_EINT | GPIO_PORTG2 | GPIO_PIN6)
#define GPIO_WEXT_INTG27   (GPIO_EINT | GPIO_PORTG2 | GPIO_PIN7)
#define GPIO_WEXT_INTA00   (GPIO_EINT | GPIO_PORTA0 | GPIO_PIN0)
#define GPIO_WEXT_INTA01   (GPIO_EINT | GPIO_PORTA0 | GPIO_PIN1)
#define GPIO_WEXT_INTA02   (GPIO_EINT | GPIO_PORTA0 | GPIO_PIN2)

/* XGPIO */
#define GPIO_XGPIO1        (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG0 | GPIO_PIN1)
#define GPIO_XGPIO2        (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG0 | GPIO_PIN2)
#define GPIO_XGPIO3        (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG0 | GPIO_PIN3)
#define GPIO_XGPIO8        (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG1 | GPIO_PIN0)
#define GPIO_XGPIO9        (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG1 | GPIO_PIN1)
#define GPIO_XGPIO10       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG1 | GPIO_PIN2)
#define GPIO_XGPIO11       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG1 | GPIO_PIN3)
#define GPIO_XGPIO12       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG1 | GPIO_PIN4)
#define GPIO_XGPIO13       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG1 | GPIO_PIN5)
#define GPIO_XGPIO14       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG1 | GPIO_PIN6)
#define GPIO_XGPIO15       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG1 | GPIO_PIN7)
#define GPIO_XGPIO16       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG2 | GPIO_PIN0)
#define GPIO_XGPIO17       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG2 | GPIO_PIN1)
#define GPIO_XGPIO18       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG2 | GPIO_PIN2)
#define GPIO_XGPIO19       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG2 | GPIO_PIN3)
#define GPIO_XGPIO20       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG2 | GPIO_PIN4)
#define GPIO_XGPIO21       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG2 | GPIO_PIN5)
#define GPIO_XGPIO22       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG2 | GPIO_PIN6)
#define GPIO_XGPIO23       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG2 | GPIO_PIN7)
#define GPIO_XGPIO24       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG3 | GPIO_PIN0)
#define GPIO_XGPIO25       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG3 | GPIO_PIN1)
#define GPIO_XGPIO26       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG3 | GPIO_PIN2)
#define GPIO_XGPIO27       (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTG3 | GPIO_PIN3)
#define GPIO_XEINT0        (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTA0 | GPIO_PIN0)
#define GPIO_XEINT1        (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTA0 | GPIO_PIN1)
#define GPIO_XEINT2        (GPIO_INPUT | GPIO_PULLDOWN | GPIO_PORTA0 | GPIO_PIN2)

#endif							/* __ARCH_ARM_SRC_S5J_CHIP_S5JT200_PINCONFIG_H__ */
