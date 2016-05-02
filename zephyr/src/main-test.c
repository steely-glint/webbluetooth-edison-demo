#include <zephyr.h>
#include <device.h>
#include <pwm.h>
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

#define IO3_RED   "PWM_0"
#define IO5_GREEN "PWM_1"
#define IO6_BLUE  "PWM_2"

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
#define BUFFER_SIZE 40

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

#define SLEEPTICKS  SECONDS(1)

void main(void)
{
  struct nano_timer timer;
	uint32_t data[2] = {0, 0};

  struct device *red, *green, *blue, *tmp36;

  PRINT("Zephyr WebBluetooth demo\n");

  red = device_get_binding(IO3_RED);
  green = device_get_binding(IO5_GREEN);
  blue = device_get_binding(IO6_BLUE);

  if (!red || !green || !blue) {
    PRINT("Cannot find LED connected to pin 3 (red), 5 (green) and 6 (blue).\n");
  }

  tmp36 = device_get_binding(ADC_DEVICE_NAME);
  if (!tmp36) {
    PRINT("Cannot find the TMP36 connected to pin A0.\n");
  }

	nano_timer_init(&timer, data);
	adc_enable(tmp36);

  while (1) {
    pwm_pin_set_values(red, 1024, SLEEPTICKS, 0);

    if (adc_read(tmp36, &table) == 0) {
      uint32_t length = BUFFER_SIZE;
      uint8_t *buf = buffer;
      for (; length > 0; length -= 4, buf += 4) {
		    uint32_t rawValue = *((uint32_t *) buf);
        float voltage = (rawValue / 1024.0) * 5.0;
        float celsius = (voltage - 0.5) * 100;
        PRINT("Celsius %f\n", celsius);
	    }
    }

    nano_timer_start(&timer, SLEEPTICKS);
    nano_timer_test(&timer, TICKS_UNLIMITED);
  }

  adc_disable(tmp36);
}