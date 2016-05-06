#include <zephyr.h>
#include <device.h>
#include <sys_clock.h>
#include <misc/byteorder.h>
#include <adc.h>

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif

#define SLEEPTICKS  SECONDS(1)

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

uint8_t tmp36_read(void)
{
    uint8_t *buf = buffer;

    // ADC only works on ARC: https://www.zephyrproject.org/doc/board/arduino_101.html
    // below two lines is testing code:
    uint32_t *set = (uint32_t*) buf;
    set[0] = 150;
    //if (adc_read(tmp36, &table) == 0) {
        uint32_t length = BUFFER_SIZE;
        for (; length > 0; length -= 4, buf += 4) {
            uint32_t rawValue = *((uint32_t *) buf);
            printf("raw temperature value %d\n\n\n", rawValue);
            float voltage = (rawValue / 1024.0) * 5.0;
            float celsius = (voltage - 0.5) * 100;
            return (uint8_t) celsius + 0.5;
        }
    //}
    return 20;
}

void main(void)
{
  struct nano_timer timer;
	uint32_t data[2] = {0, 0};

  struct device *tmp36;

  PRINT("Zephyr WebBluetooth demo\n");

  if ((tmp36 = device_get_binding(ADC_DEVICE_NAME)) == NULL) {
      printk("device_get_binding: failed for ADC\n");
      printk("Temperature (celsius): %d\n\n\n", tmp36_read());
  }

	nano_timer_init(&timer, data);
	adc_enable(tmp36);

  while (1) {
    tmp36_read();

    nano_timer_start(&timer, SLEEPTICKS);
    nano_timer_test(&timer, TICKS_UNLIMITED);
  }

  adc_disable(tmp36);
}