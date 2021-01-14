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

#ifndef __AMEBAD_I2S_H
#define __AMEBAD_I2S_H

/************************************************************************************
 * Included Files
 ************************************************************************************/

#include <tinyara/config.h>
#include <tinyara/audio/i2s.h>

#include <arch/chip/irq.h>

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/************************************************************************************
 * Pre-processor Definitions
 ************************************************************************************/
/* SAI module features */

/************************************************************************************
 * Public Data
 ************************************************************************************/
/**
* @brief I2S Peripheral, 0 & 1.
*
*/
typedef enum {
	I2S_NUM_0 = 0x0,			/*!< I2S 0 */
	//I2S_NUM_1 = 0x1,			/*!< I2S 1 */
	I2S_NUM_MAX,
} i2s_port_t;
/**
* @brief I2S bit width per sample.
*
*/
typedef enum {
	I2S_BITS_PER_SAMPLE_8BIT  = 8,  /*!< I2S bits per sample: 8-bits */
	I2S_BITS_PER_SAMPLE_16BIT = 16, /*!< I2S bits per sample: 16-bits */
	I2S_BITS_PER_SAMPLE_24BIT = 24, /*!< I2S bits per sample: 24-bits */
	I2S_BITS_PER_SAMPLE_32BIT = 32, /*!< I2S bits per sample: 32-bits */
} i2s_bits_per_sample_t;
/****************************************************************************
 * Name: amebad_i2s_initialize
 *
 * Description:
 *   Initialize the selected I2S port.
 *
 * Input Parameter:
 *   Port number (for hardware that has mutiple I2S interfaces)
 *
 * Returned Value:
 *   Valid I2S device structure reference on succcess; a NULL on failure
 *
 ****************************************************************************/

FAR struct i2s_dev_s *amebad_i2s_initialize(uint16_t port);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif							/* __ASSEMBLY__ */
#endif							/* __AMEBAD_I2S_H */
