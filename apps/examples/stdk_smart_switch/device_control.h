/* ***************************************************************************
 *
 * Copyright 2020 Samsung Electronics All Rights Reserved.
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

#include <unistd.h>
#include <tinyara/clock.h>
#include <tinyara/config.h>
#include <iotbus/iotbus_gpio.h>

#if defined(CONFIG_ARCH_BOARD_ARTIK05X_FAMILY)
#define GPIO_OUTPUT_NOTIFICATION_LED 45
#define GPIO_INPUT_BUTTON 44
#define GPIO_OUTPUT_MAINLED 53

#define GPIO_INPUT_BUTTON_EDGE IOTBUS_GPIO_EDGE_FALLING

enum notification_led_gpio_state {
	NOTIFICATION_LED_GPIO_ON = IOTBUS_GPIO_LOW,
	NOTIFICATION_LED_GPIO_OFF = IOTBUS_GPIO_HIGH,
};

enum button_gpio_state {
	BUTTON_GPIO_RELEASED = IOTBUS_GPIO_HIGH,
	BUTTON_GPIO_PRESSED = IOTBUS_GPIO_LOW,
};

#else //default esp32

#define GPIO_OUTPUT_NOTIFICATION_LED 4
#define GPIO_INPUT_BUTTON 5
#define GPIO_OUTPUT_MAINLED 18

#if defined(CONFIG_ARCH_BOARD_ESP32_DEVKITC)
#define GPIO_INPUT_BUTTON_EDGE IOTBUS_GPIO_EDGE_FALLING
enum button_gpio_state {
	BUTTON_GPIO_RELEASED = IOTBUS_GPIO_HIGH,
	BUTTON_GPIO_PRESSED = IOTBUS_GPIO_LOW,
};
#else
#define GPIO_INPUT_BUTTON_EDGE IOTBUS_GPIO_EDGE_RISING
enum button_gpio_state {
	BUTTON_GPIO_RELEASED = IOTBUS_GPIO_LOW,
	BUTTON_GPIO_PRESSED = IOTBUS_GPIO_HIGH,
};
#endif

enum notification_led_gpio_state {
	NOTIFICATION_LED_GPIO_ON = IOTBUS_GPIO_LOW,
	NOTIFICATION_LED_GPIO_OFF = IOTBUS_GPIO_HIGH,
};

#endif

#define LED_BLINK_TIME 50
//just make sleep as expected
#define portTICK_PERIOD_MS 1
#undef vTaskDelay
#define vTaskDelay(t) usleep((t) * 1000)

enum led_animation_mode_list {
	LED_ANIMATION_MODE_IDLE = 0,
	LED_ANIMATION_MODE_FAST,
	LED_ANIMATION_MODE_SLOW,
};

#define BUTTON_DEBOUNCE_TIME_MS 20
#define BUTTON_LONG_THRESHOLD_MS 2500
#define BUTTON_DELAY_MS 300

enum button_event_type {
	BUTTON_LONG_PRESS = 0,
	BUTTON_SHORT_PRESS = 1,
};

enum main_led_gpio_state {
	MAINLED_GPIO_ON = IOTBUS_GPIO_HIGH,
	MAINLED_GPIO_OFF = IOTBUS_GPIO_LOW,
};


void button_isr_handler(void *arg);
bool get_button_event(int *button_event_type, int *button_event_count);
void led_blink(int gpio, int delay, int count);
void change_led_state(int noti_led_mode);
void gpio_init(void);
int gpio_set_level(int port, int value);
