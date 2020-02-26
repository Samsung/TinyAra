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
/****************************************************************************
 * os/arch/arm/src/amebad/amebad_gpio_lowerhalf.c
 ****************************************************************************/
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <tinyara/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <debug.h>
#include <errno.h>

#include <arch/irq.h>
#include <tinyara/gpio.h>

#include "up_arch.h"
#include "gpio_api.h"
#include "tinyara/gpio.h"

#include "objects.h"
#include "PinNames.h"
#include "gpio_irq_api.h"
#include "gpio_irq_ex_api.h"
#include <tinyara/kmalloc.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/
struct amebad_lowerhalf_s {
	FAR const struct gpio_ops_s *ops;
	struct gpio_upperhalf_s *parent;

	/* Including private value */
	gpio_t obj;
	u32 pinmode;
	u32 pinpull;
	gpio_handler_t handler;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#ifndef CONFIG_DISABLE_POLL
void amebad_gpio_interrupt(uint32_t arg, gpio_irq_event event)
{
	struct amebad_lowerhalf_s *lower = (struct amebad_lowerhalf_s *)arg;

	if (lower->handler != NULL) {
		DEBUGASSERT(lower->handler != NULL);
		lower->handler(lower->parent);
	}
	return OK;
}
#endif

/****************************************************************************
 * Name: amebad_gpio_set
 *
 * Description:
 *   Set GPIO value
 *
 * Input Parameters:
 *   lower - lowerhalf GPIO driver
 *   value - value to set
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/
static void amebad_gpio_set(FAR struct gpio_lowerhalf_s *lower,
						 FAR unsigned int value)
{
	struct amebad_lowerhalf_s *priv = (struct amebad_lowerhalf_s *)lower;
	gpio_write(&priv->obj, value);
}

/****************************************************************************
 * Name: amebad_gpio_get
 *
 * Description:
 *   Get GPIO value
 *
 * Input Parameters:
 *   lower - lowerhalf GPIO driver
 *
 * Returned Value:
 *   >= 0: gpio value
 *   == -EINVAL: invalid gpio
 ****************************************************************************/
static int amebad_gpio_get(FAR struct gpio_lowerhalf_s *lower)
{
	struct amebad_lowerhalf_s *priv = (struct amebad_lowerhalf_s *)lower;
	return gpio_read(&priv->obj);
}


static int amebad_gpio_setdir(FAR struct gpio_lowerhalf_s *lower,
						   unsigned long arg)
{
	struct amebad_lowerhalf_s *priv = (struct amebad_lowerhalf_s *)lower;
	
	if (arg == GPIO_DIRECTION_OUT) {
		priv->pinmode=PIN_OUTPUT;
		gpio_change_dir(&priv->obj,priv->pinmode);
	} else {
		priv->pinmode=PIN_INPUT;
		gpio_change_dir(&priv->obj,priv->pinmode);
	}

	return OK;
}

static int amebad_gpio_pull(FAR struct gpio_lowerhalf_s *lower, unsigned long arg)
{
	struct amebad_lowerhalf_s *priv = (struct amebad_lowerhalf_s *)lower;

	u32 GPIO_PuPd;

	if (arg == GPIO_DRIVE_FLOAT) {
		GPIO_PuPd = GPIO_PuPd_NOPULL;
	} else if (arg == GPIO_DRIVE_PULLUP) {
		GPIO_PuPd = GPIO_PuPd_UP;
	} else if (arg == GPIO_DRIVE_PULLDOWN) {
		GPIO_PuPd = GPIO_PuPd_DOWN;
	} else {
		return -EINVAL;
	}
	
	priv->pinpull=GPIO_PuPd;
	gpio_pull_ctrl(&priv->obj, GPIO_PuPd);
	
	return OK;
}

static int amebad_gpio_enable(FAR struct gpio_lowerhalf_s *lower, int falling,
						   int rising, gpio_handler_t handler)
{	
	struct amebad_lowerhalf_s *priv = (struct amebad_lowerhalf_s *)lower;
	gpio_irq_event event;
	
	if (falling && rising) {
		event = IRQ_FALL_RISE;
	} else if (falling) {
		event = IRQ_FALL;
	} else if (rising) {
		event = IRQ_RISE;
	} else {
		handler = NULL;
	}
	priv->handler = handler;

	if (handler) {
		gpio_irq_init(&priv->obj, priv->obj.pin, amebad_gpio_interrupt,(uint32_t) priv);
		gpio_irq_set(&priv->obj, event, 1);
		gpio_irq_enable(&priv->obj);
	} else {
		gpio_irq_disable(&priv->obj);
		gpio_irq_set(&priv->obj, event, 0);
		gpio_irq_deinit(&priv->obj);
	}

	return OK;
}

/****************************************************************************
 * Private Data
 ****************************************************************************/
static const struct gpio_ops_s amebad_gpio_ops = {
	.set    = amebad_gpio_set,
	.get    = amebad_gpio_get,
	.pull   = amebad_gpio_pull,
	.setdir = amebad_gpio_setdir,
	.enable = amebad_gpio_enable,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: amebad_gpio_lowerhalf
 *
 * Description:
 *   Instantiate the GPIO lower half driver for the amebad.
 *   General usage:
 *
 *     #include <tinyara/gpio.h>
 *     #include "amebad_gpio.h"
 *
 *     struct gpio_lowerhalf_s *lower;
 *     lower = amebad_gpio_lowerhalf();
 *     gpio_register(0, lower);
 *
 * Input Parameters:
 *   
 *
 * Returned Value:
 *   On success, a non-NULL GPIO lower interface is returned. Otherwise, NULL.
 *
 ****************************************************************************/
FAR struct gpio_lowerhalf_s *amebad_gpio_lowerhalf(u32 pinname, u32 pinmode, u32 pinpull)
{	
	struct amebad_lowerhalf_s *lower;
	
	lower = (struct amebad_lowerhalf_s *)kmm_malloc(sizeof(struct amebad_lowerhalf_s));
	if (!lower) {
		return NULL;
	}
	gpio_init(&lower->obj, pinname);
    	gpio_dir(&lower->obj, pinmode);
    	gpio_mode(&lower->obj, pinpull);
	lower->pinmode =pinmode;
	lower->pinpull = pinpull;
	lower->ops = &amebad_gpio_ops;
	return (struct gpio_lowerhalf_s *)lower;
}
