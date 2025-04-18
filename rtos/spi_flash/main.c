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


static const char *cap[3] = {
    "W25X16",
    "W25X32",
    "W25X64"
    "W25X128"
};

static void receive_task(void *args __attribute__((unused))) {
    char ch;
    int devx;
    const char * device;
    uint32_t info;
    for (;;) {
        ch = usb_getc();
        if (ch == '1') {
            w25_power(SPI2, 1);
            break;
        } else if (ch == 'I') {
            info = w25_manuf_device(SPI2);
            devx = (int)(info & 0xff)-0x14;
            if(devx<4) {
              device = cap[devx];
            } else{
              device = "Unknown";
            }
            usb_printf("Manufacturer $%02X Device $%02X (%s)\n",
                (uint16_t)info>>8,(uint16_t)info&0xFF,
                device);
        }
    }
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
    led_setup();
    spi_setup();
    usb_start();
    xTaskCreate(receive_task, "receive", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-2, NULL);
    vTaskStartScheduler();
    for(;;);
    return 0;
}
