#include "FreeRTOS.h"
#include "task.h"
#include "usbcdc.h"
#include "spi_flash.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <time.h>
#include <string.h>
#include <stdio.h>


static char *cap[4] = {
    "W25X16",
    "W25X32",
    "W25X64",
    "W25X128"
};

static void send_task(void *args __attribute__((unused))) {
    char ch;
    int devx;
    char * devs;
    char * device;
    uint16_t info;
    char str[5];
    w25_power(SPI2,1);
    for(;;) {
      ch = usb_getc();
      if (ch == '1') {
          info = w25_manuf_device(SPI2);
          devx = (int)(info & 0xFF)-0x14;
          if ( devx < 4 ){
              device = cap[devx];
          } else{
              device = "unknown";
          }
//          str[0] = "0123456789ABCDEF"[(info >> 12) & 0x0F];
//          str[1] = "0123456789ABCDEF"[(info >> 8) & 0x0F];
//          str[2] = "0123456789ABCDEF"[(info >> 4) & 0x0F];
//          str[3] = "0123456789ABCDEF"[info & 0x0F];
//          str[4] = '\0';
          usb_puts(device);
      }
    }
    for (;;);
}

static void led_task(void *args __attribute__((unused))) {
  for(;;) {
      gpio_toggle(GPIOE, GPIO5);
      vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

static void led_setup() {
    rcc_periph_clock_enable(RCC_GPIOE);
    gpio_set_mode(GPIOE,GPIO_MODE_OUTPUT_2_MHZ,
              GPIO_CNF_OUTPUT_PUSHPULL,GPIO5);
}


int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    usb_start();
    led_setup();
    spi_setup();
    xTaskCreate(send_task, "send", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);
    vTaskStartScheduler();
    for(;;);
    return 0;
}
