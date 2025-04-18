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
    uint32_t info;
    char str[5];
    for(;;) {
      ch = usb_getc();
      switch (ch) {
        case '0':
            w25_power(SPI2,0);
            usb_puts("w25q128 already poweroff\n");
            break;
        case '1':
            w25_power(SPI2,1);
            usb_puts("w25q128 already poweron\n");
            break;
        case 'I':
            info = w25_manuf_device(SPI2);
            devx = (int)(info & 0xFF)-0x14;
            if ( devx < 4 ){
                device = cap[devx];
            } else{
                device = "unknown";
            }
            usb_puts(device);
            break;
        case 'J':
              info = w25_JEDEC_ID(SPI2);
              devx = (int)(info & 0xFF)-0x15;	// Offset is 1 higher here
              if ( devx < 4 )
                  device = cap[devx];
              else	device = "unknown";
              usb_printf("Manufacturer $%02X Type $%02X Capacity $%02X (%s)\n",
                  (uint16_t)(info>>16),
                  (uint16_t)((info>>8)&0xFF),
                  (uint16_t)(info&0xFF),
                  device);
              break;
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
