#include <zephyr.h>
#include <device.h>
#include <pwm.h>
#include <sys_clock.h>

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

#define SLEEPTICKS  SECONDS(1)

void main(void)
{
  struct nano_timer timer;
  void *timer_data[1];

  struct device *red, *green, *blue;

  nano_timer_init(&timer, timer_data);

  PRINT("Zephyr WebBluetooth demo\n");

  red = device_get_binding(IO3_RED);
  green = device_get_binding(IO5_GREEN);
  blue = device_get_binding(IO6_BLUE);

  if (!red || !green || !blue) {
    PRINT("Cannot find LED connected to pin 3 (red), 5 (green) and 6 (blue)!\n");
  }

  while (1) {
    pwm_pin_set_values(red, 1024, SLEEPTICKS, 0);

    nano_timer_start(&timer, SLEEPTICKS);
    nano_timer_test(&timer, TICKS_UNLIMITED);
  }
}