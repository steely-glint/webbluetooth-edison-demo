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

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#define BT_UUID_WEBBT BT_UUID_DECLARE_16(0xfc00)

#define BT_UUID_TEMP BT_UUID_DECLARE_16(0xfc0a)
#define BT_UUID_RGB BT_UUID_DECLARE_16(0xfc0b)

#define SENSOR_1_NAME "Temperature"
#define SENSOR_2_NAME "Led"

static struct bt_gatt_ccc_cfg  blvl_ccc_cfg[CONFIG_BLUETOOTH_MAX_PAIRED] = {};
static uint8_t simulate_blvl;
static uint8_t temperature = 100;
static uint8_t rgb[3] = {0xff, 0x01, 0x02}; // red

struct device *pwm, *tmp36;
#define IO3_RED   0
#define IO5_GREEN 1
#define IO6_BLUE  2

#define ADC_DEVICE_NAME "ADC_0"

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

	printk("read_rgb: rgb=%x %x %x size=%d %d\n", 
		(uint8_t)rgb[0], rgb[1], rgb[2], sizeof(rgb), len);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, rgb,
				 sizeof(rgb));
}

static ssize_t write_rgb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, uint16_t len, uint16_t offset)
{
	memcpy(rgb, buf, sizeof(rgb));

	printk("write_rgb: rgb=%x %x %x size=%d %d\n", 
		(uint8_t)rgb[0], rgb[1], rgb[2], sizeof(rgb), len);

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

void service_init(void)
{
	if ((pwm = device_get_binding("PWM")) == NULL)
		printk("device_get_binding: failed for PWM\n");
	
	bt_gatt_register(attrs, ARRAY_SIZE(attrs));
}

void service_notify(void)
{
	if (!simulate_blvl) {
		return;
	}

	temperature--;
	if (!temperature) {
		/* Software eco temperature charger */
		temperature = 100;
	}

	bt_gatt_notify(NULL, &attrs[2], &temperature, sizeof(temperature));
}
