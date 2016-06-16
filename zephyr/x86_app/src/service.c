/** @file
 *  @brief BAS Service sample
 */

/*
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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <device.h>
#include <pwm.h>
#include <adc.h>

#include <ipm.h>
#include <ipm/ipm_quark_se.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <gpio.h>
#include <sys_clock.h>

QUARK_SE_IPM_DEFINE(temperature_ipm, 0, QUARK_SE_IPM_INBOUND);

#define BT_UUID_WEBBT BT_UUID_DECLARE_16(0xfc00)

#define BT_UUID_TEMP BT_UUID_DECLARE_16(0xfc0a)
#define BT_UUID_RGB BT_UUID_DECLARE_16(0xfc0b)

#define SENSOR_1_NAME "Temperature"
#define SENSOR_2_NAME "Led"

static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};
static uint8_t simulate_blvl;
static uint8_t temperature = 20;
static uint8_t rgb[3] = {0xff, 0x00, 0x00}; // red

struct device *pwm, *tmp36, *tmp2302;
#define IO3_RED   0
#define IO5_GREEN 1
#define IO6_BLUE  2

#define ADC_DEVICE_NAME "ADC_0"

#define SLEEPTICKS      SECONDS(1)
/* about 2 ms, thus 500Hz */
#define PERIOD 64000
#define MAX(a,b) (((a)>(b))?(a):(b))

#if defined(CONFIG_GPIO_DW_0)
#define GPIO_DRV_NAME CONFIG_GPIO_DW_0_NAME
#elif defined(CONFIG_GPIO_QMSI_0)
#define GPIO_DRV_NAME CONFIG_GPIO_QMSI_0_NAME
#elif defined(CONFIG_GPIO_ATMEL_SAM3)
#define GPIO_DRV_NAME CONFIG_GPIO_ATMEL_SAM3_PORTB_DEV_NAME
#elif
#define GPIO_DRV_NAME "GPIO_0"
#endif

#define IO8_AM2302 16

uint32_t start_time;
uint32_t stop_time;
uint32_t cycles_spent;

void pwm_write(void)
{
    uint32_t r_on, g_on, b_on;
    uint32_t r_off, g_off, b_off;

    r_on = (rgb[0] / 255.0) * PERIOD;
    g_on = (rgb[1] / 255.0) * PERIOD;
    b_on = (rgb[2] / 255.0) * PERIOD;

    r_on = MAX(1, r_on);
    g_on = MAX(1, g_on);
    b_on = MAX(1, b_on);

    r_off = PERIOD - r_on;
    g_off = PERIOD - g_on;
    b_off = PERIOD - b_on;

    r_off = MAX(1, r_off);
    g_off = MAX(1, g_off);
    b_off = MAX(1, b_off);

    printk("rgb: %x %x %x\n", rgb[0], rgb[1], rgb[2]);
    printk("on : %x %x %x\n", r_on, g_on, b_on);
    printk("off: %x %x %x\n", r_off, g_off, b_off);

    pwm_pin_set_values(pwm, IO3_RED, r_on, r_off);
    pwm_pin_set_values(pwm, IO5_GREEN, g_on, g_off);
    pwm_pin_set_values(pwm, IO6_BLUE, b_on, b_off);
}

static void blvl_ccc_cfg_changed(uint16_t value)
{
    simulate_blvl = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_temperature(struct bt_conn *conn, const struct bt_gatt_attr *attr,
    void *buf, uint16_t len, uint16_t offset)
{
    const char *value = attr->user_data;

    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
        sizeof(*value));
}

static ssize_t read_rgb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
    void *buf, uint16_t len, uint16_t offset)
{
    memcpy(rgb, attr->user_data, sizeof(rgb));

    printk("read_rgb: %x %x %x\n", rgb[0], rgb[1], rgb[2]);

    return bt_gatt_attr_read(conn, attr, buf, len, offset, rgb,
                 sizeof(rgb));
}

static ssize_t write_rgb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
    const void *buf, uint16_t len, uint16_t offset)
{
    memcpy(rgb, buf, sizeof(rgb));

    printk("write_rgb: %x %x %x\n", rgb[0], rgb[1], rgb[2]);

    pwm_write();

    return sizeof(rgb);
}

/* WebBT Service Declaration */
static struct bt_gatt_attr attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BT_UUID_WEBBT),

    /* Temperature */
    BT_GATT_CHARACTERISTIC(BT_UUID_TEMP,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY),
    BT_GATT_DESCRIPTOR(BT_UUID_TEMP, BT_GATT_PERM_READ,
        read_temperature, NULL, &temperature),
    BT_GATT_CUD(SENSOR_1_NAME, BT_GATT_PERM_READ),
    BT_GATT_CCC(blvl_ccc_cfg, blvl_ccc_cfg_changed),

    /* RGB Led */
    BT_GATT_CHARACTERISTIC(BT_UUID_RGB,
        BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE),
    BT_GATT_DESCRIPTOR(BT_UUID_RGB, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
        read_rgb, write_rgb, rgb),
    BT_GATT_CUD(SENSOR_2_NAME, BT_GATT_PERM_READ),
};

void temperature_ipm_callback(void *context, uint32_t id, volatile void *data)
{
    struct bt_conn *conn = (struct bt_conn *) context;
    temperature = *(uint8_t*) data;
    printk("Got temperature from ARC %d\n", temperature);
    bt_gatt_notify(conn, &attrs[2], &temperature, sizeof(temperature));
}

uint32_t measure_signal(uint32_t signal)
{
    uint32_t count = 1;

    uint32_t readValue;

    start_time = sys_cycle_get_32();

    do {
      gpio_pin_read(tmp2302, IO8_AM2302, &readValue);
      if (count++ > 255) return -1;
    } while (readValue == signal);

    stop_time = sys_cycle_get_32();

    cycles_spent = stop_time - start_time;
    return SYS_CLOCK_HW_CYCLES_TO_NS(cycles_spent) / 1000; // Microseconds
}

int verifyResponse(uint32_t response)
{
    if (response >= 75 && response <= 85)
        return 1;
    return -1;
}

int verifyLowSignal(uint32_t low)
{
    if (low >= 48 && low <= 55)
        return 1;
    return -1;
}

int checkHighSignal(uint32_t high)
{
    if (high >= 68 && high <= 75)
        return 1;
    if (high >= 22 && high <= 30)
        return 0;
    return -1;
}

void read_am2302_tmp()
{
    uint32_t cycles[80];

    gpio_pin_configure(tmp2302, IO8_AM2302, (GPIO_DIR_OUT));
    gpio_pin_write(tmp2302, IO8_AM2302, 1);
    task_sleep(USEC(1000));

   // STEP 1+2

   // We set MCU to output.
   // Due to previous pullup, it should always be high.

   // We signal the AM2302 to wake up, by signaling
   // low for min 800us, but typically 1ms. (max 20ms).
   gpio_pin_write(tmp2302, IO8_AM2302, 0);
   task_sleep(USEC(1000));

   // End the start signal by setting data line high for 40 microseconds.
   gpio_pin_write(tmp2302, IO8_AM2302, 1);
   task_sleep(USEC(40));

   // Then set MCU to input and release the bus
   // Due to pullup, it will go high and it will take
   // at least 20us, though typically 30ns and
   // max 200ns for the AM2302 to signal low.
   gpio_pin_configure(tmp2302, IO8_AM2302, (GPIO_DIR_IN));
   task_sleep(USEC(10));

   // Signal from AM2302 that it is ready to send data.

   uint32_t time;

   time = measure_signal(0);
   if (verifyResponse(time) < 0)
       printk("Response low time not within limits: %d\n", time);
   time = measure_signal(1);
   if (verifyResponse(time) < 0)
       printk("Response high time not within limits: %d\n", time);

   // Read data.
   for (int i = 0; i < 80; i += 2) {
       cycles[i] = measure_signal(0);
       cycles[i+1] = measure_signal(1);
   }

   // -- Timing critical code is now over. --

   uint8_t dataset[5];

   // Each data set is 5 bytes (5 * 8 = 40)
   for (int i = 0; i < 40; ++i) {
       uint32_t low = cycles[2*i];
       uint32_t high = cycles[2*i+1];

       dataset[i/8] <<= 1; // Move existing value one bit.

       int lowstate = verifyLowSignal(low);
       int highstate = checkHighSignal(high);

       if (highstate == 1)
           dataset[i/8] |= 1;

       if (lowstate < 0 || highstate < 0) printk("X: "); else printk("%d: ", highstate);
       printk("low"); if (lowstate < 0) printk("*"); printk("\t: %d\t/\t", low);
       printk("high"); if (highstate < 0) printk("*"); printk("\t: %d\n", high);
       if ((i + 1) % 8 == 0) printk("\n");
    }

    // Debugging:
    for (int j = 0; j < 5; j++){
        printk("%d ", dataset[j]);
    }
    printk("\n");

    // Checksum check:
    if (dataset[4] != ((dataset[0] + dataset[1] + dataset[2] + dataset[3]) & 0xFF)) {
        printk("Checksum failed!\n");
    }

    float humidity = (float) (dataset[0] << 8 | dataset[1]) / 10.0;
    printk("Humidity: %f \n", humidity);
    float temperature;

    int neg = dataset[2] & 0x80; // higtest bit only.
    dataset[2] &= 0x7F; // Remove highest bit.

    temperature = (float) (dataset[2] << 8 | dataset[3]) / 10.0;
    if (neg > 0)
        temperature *= -1;

    printk("Temperature: %f \n", temperature);
}

void service_init(struct bt_conn *conn)
{
    if ((pwm = device_get_binding("PWM_0")) == NULL)
        printk("device_get_binding: failed for PWM\n");
    else {
        pwm_write();
        printk("PWM init OK\n");
    }

    struct device *ipm;

    ipm = device_get_binding("temperature_ipm");
    ipm_register_callback(ipm, temperature_ipm_callback, conn);
    ipm_set_enabled(ipm, 1);

    bt_gatt_register(attrs, ARRAY_SIZE(attrs));

    if((tmp2302 = device_get_binding(GPIO_DRV_NAME)) == NULL)
        printk("device_get_binding: failed for TMP2302\n");
    else {
        read_am2302_tmp();
    }
}