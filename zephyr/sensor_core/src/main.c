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

#include <gpio.h>
#include <sys_clock.h>

#if defined(CONFIG_GPIO_DW_0)
#define GPIO_DRV_NAME CONFIG_GPIO_DW_0_NAME
#elif defined(CONFIG_GPIO_QMSI_0)
#define GPIO_DRV_NAME CONFIG_GPIO_QMSI_0_NAME
#elif defined(CONFIG_GPIO_ATMEL_SAM3)
#define GPIO_DRV_NAME CONFIG_GPIO_ATMEL_SAM3_PORTB_DEV_NAME
#else
#define GPIO_DRV_NAME "GPIO_0"
#endif

#if defined(CONFIG_SOC_QUARK_SE_SS)
#define A1_AM2302	3
#define GPIO_NAME   "GPIO_SS_"
#elif defined(CONFIG_SOC_QUARK_SE)
#define A1_AM2302	16
#define GPIO_NAME   "GPIO_"
#elif defined(CONFIG_SOC_QUARK_D2000)
#define A1_AM2302	8
#define GPIO_NAME   "GPIO_"
#endif

uint32_t start_time;
uint32_t stop_time;
uint32_t cycles_spent;

struct device *pwm, *tmp36, *tmp2302;

static int32_t temperature = 0;
static uint32_t humidity = 0;

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
        float voltage = (rawValue / 4096.0) * 3.3;
        float celsius = (voltage - 0.5) * 100;
        return (uint8_t) celsius + 0.5;
    }

    return 20;
}

#define DHT_SIGNAL_MAX_WAIT_DURATION 100

uint8_t measure_signal(uint32_t signal)
{
    uint32_t val;
	uint32_t elapsed_cycles;
	uint32_t max_wait_cycles = (uint32_t)(
	    (uint64_t)DHT_SIGNAL_MAX_WAIT_DURATION *
	    (uint64_t)sys_clock_hw_cycles_per_sec /
        (uint64_t)USEC_PER_SEC
    );
    uint32_t start_cycles = sys_cycle_get_32();

    do {
        gpio_pin_read(tmp2302, A1_AM2302, &val);
        elapsed_cycles = sys_cycle_get_32() - start_cycles;

        if (elapsed_cycles >= max_wait_cycles)
            return -1;
    } while (val == signal);

    return (uint64_t)elapsed_cycles *
	    (uint64_t)USEC_PER_SEC /
        (uint64_t)sys_clock_hw_cycles_per_sec;
}

void am2302_tmp_init()
{
    gpio_pin_configure(tmp2302, A1_AM2302, GPIO_DIR_OUT);
    gpio_pin_write(tmp2302, A1_AM2302, 1);
}

void read_am2302_tmp(int32_t *temperature, uint32_t *humidity)
{
    uint32_t durations[40];
    am2302_tmp_init();

    // Send start signal
    start_time = sys_cycle_get_32();
    gpio_pin_write(tmp2302, A1_AM2302, 0);

    sys_thread_busy_wait(1800);

    // ...then the MCU will pulls up and wait 20-40us for AM2302's response
    gpio_pin_write(tmp2302, A1_AM2302, 1);

    gpio_pin_configure(tmp2302, A1_AM2302, GPIO_DIR_IN);

   // Signal from AM2302 that it is ready to send data.
   if (measure_signal(1) < 0)
       printk("Timeout from sensor\n");
   if (measure_signal(0) < 0)
       printk("Timeout for low signal response\n");
   if (measure_signal(1) < 0)
       printk("timeout for high signal response\n");

   // Read data.
   for (int i = 0; i < 40; i++) {
     if (measure_signal(0) < 0) {
        printk("Timeout for low signal measurement\n");
     }
     if ((durations[i] = measure_signal(1)) < 0)
        printk("Timeout for high signal measurement\n");
   }

   // -- Timing critical code is now over. --
   uint8_t dataset[5];

   // Calculate average due to lossy precision of the measurements
   uint8_t min, max, avg;
   min = durations[0];
   max = durations[0];
   for (int i = 1; i < 40; ++i) {
      if (min > durations[i])
         min = durations[i];
      if (max < durations[i])
         max = durations[i];
   }
   avg = ((int16_t) min + (int16_t) max) / 2;

   // Each data set is 5 bytes (5 * 8 = 40)
   for (int i = 0; i < 40; ++i) {
       uint32_t duration = durations[i];
       dataset[i/8] <<= 1; // Move existing value one bit.
       if (duration >= avg)
           dataset[i/8] |= 1;
    }

    // Debugging:
    //printk("Dataset:");
    //for (int j = 0; j < 5; j++)
    //    printk(" %d", dataset[j]);
    // printk("\n");

    // Checksum check:
    if (dataset[4] != ((dataset[0] + dataset[1] + dataset[2] + dataset[3]) & 0xFF)) {
        printk("Checksum failed!\n");
    }

    *humidity = (dataset[0] << 8 | dataset[1]);

    int neg = dataset[2] & 0x80; // highest bit only.
    dataset[2] &= 0x7F; // Remove highest bit.
    *temperature = (dataset[2] << 8 | dataset[3]);
    if (neg > 0)
        *temperature *= -1;

    printk("Humidity: %d\tTemperature: %d\n", *humidity, *temperature);
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

    if((tmp2302 = device_get_binding(GPIO_DRV_NAME)) == NULL)
        printk("device_get_binding: failed for TMP2302\n");
    else {
        printk("GPIO init OK\n");
    }

    struct device *ipm = device_get_binding("temperature_ipm");

    adc_enable(tmp36);
    while (1) {
        if (tmp2302)
            read_am2302_tmp(&temperature, &humidity);
        else
            temperature = tmp36_read();

        ipm_send(ipm, 1, 1, &temperature, sizeof(temperature));
        ipm_send(ipm, 1, 2, &humidity, sizeof(humidity));

		task_sleep(SLEEPTICKS);
	}
    adc_disable(tmp36);
}
