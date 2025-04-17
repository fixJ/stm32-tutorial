#include "FreeRTOS.h"
#include "task.h"
#include "usbcdc.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <time.h>
#include <string.h>
#include <stdio.h>


static void receive_task(void *args __attribute__((unused))) {
    char ch;
    for (;;) {
      ch = usb_getc();
      if (ch == '1') {
          usb_write("hello", 5);
      } else {
        usb_write("error", 5);
      }
    }
}


int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    usb_start();
    xTaskCreate(receive_task, "receive", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);
    vTaskStartScheduler();
    for(;;);
    return 0;
}
