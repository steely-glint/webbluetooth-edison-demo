/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <zephyr.h>

#include <ipm.h>
#include <ipm/ipm_quark_se.h>
#include <device.h>

#include <init.h>
#include <misc/printk.h>
#include <string.h>

static uint8_t temperature = 20;

QUARK_SE_IPM_DEFINE(temperature_ipm, 0, QUARK_SE_IPM_OUTBOUND);

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  1100
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 800)

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
{
	printk("Hello from sensor core (ARC)!\n");

	while (1) {
    struct device *ipm = device_get_binding("temperature_ipm");

    temperature--;
    if (!temperature) {
        /* Software eco temperature charger */
        temperature = 20;
    }

    uint32_t id = 1;

    ipm_send(ipm, 1, id, &temperature, sizeof(temperature));

		task_sleep(SLEEPTICKS);
	}
}
