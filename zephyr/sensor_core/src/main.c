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

#include <adc.h>

static uint8_t temperature = 20;

QUARK_SE_IPM_DEFINE(temperature_ipm, 0, QUARK_SE_IPM_OUTBOUND);

/* specify delay between greetings (in ms); compute equivalent in ticks */

#define SLEEPTIME  1100
#define SLEEPTICKS (SLEEPTIME * sys_clock_ticks_per_sec / 800)

#define ADC_DEVICE_NAME "ADC_0"

/*
 * The analog input pin and channel number mapping
 * for Arduino 101 board.
 * A0 Channel 10
 * A1 Channel 11
 * A2 Channel 12
 * A3 Channel 13
 * A4 Channel 14
 */
#define A0 10
#define BUFFER_SIZE 4

static uint8_t buffer[BUFFER_SIZE];

static struct adc_seq_entry sample = {
	.sampling_delay = 12,
	.channel_id = A0,
	.buffer = buffer,
	.buffer_length = BUFFER_SIZE,
};

static struct adc_seq_table table = {
	.entries = &sample,
	.num_entries = 1,
};

struct device *tmp36;

uint8_t tmp36_read(void)
{
    uint8_t *buf = buffer;

    if (adc_read(tmp36, &table) != 0) {
        printk("Couldn't read from tmp36\n");
    }

    uint32_t length = BUFFER_SIZE;
    for (; length > 0; length -= 4, buf += 4) {
        uint32_t rawValue = *((uint32_t *) buf);
        printk("Raw temperature value %d\n", rawValue);
        float voltage = (rawValue / 512.0) * 5.0;
        float celsius = (voltage - 0.5) * 100;
        return (uint8_t) celsius + 0.5;
    }

    return 20;
}

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
{
	printk("Hello from sensor core (ARC)!\n");

  if ((tmp36 = device_get_binding(ADC_DEVICE_NAME)) == NULL) {
      printk("device_get_binding: failed for ADC\n");
  }

  adc_enable(tmp36);
	while (1) {
    struct device *ipm = device_get_binding("temperature_ipm");

    temperature = tmp36_read();

    uint32_t id = 1;

    ipm_send(ipm, 1, id, &temperature, sizeof(temperature));

		task_sleep(SLEEPTICKS);
	}
  adc_disable(tmp36);
}
