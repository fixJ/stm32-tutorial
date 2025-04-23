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
#include <ctype.h>


void led_on(void) __attribute__ ((noinline, section(".ov_led_on")));
void led_off(void) __attribute__ ((noinline, section(".ov_led_off")));


typedef struct {
      short    regionx;
      void    *vma;
      char    *start;
      char    *stop;
      unsigned long size;
      void    *func;
} s_overlay;

#define N_REGIONS 1
#define N_OVLY    2

#define OVERLAY(region, ov, sym) { region, &ov, &__load_start ## sym, &__load_stop ## sym, 0, sym }
#define LOADREF(sym) __load_start ## sym, __load_stop ## sym

extern unsigned long overlay1;
extern char LOADREF(led_on), LOADREF(led_off);
extern long __load_start_led_on, __load_start_led_off;

static s_overlay overlays[N_OVLY] = {
  OVERLAY(0, overlay1, led_on),
  OVERLAY(0, overlay1, led_off)
};


static s_overlay *cur_overlay[N_REGIONS] = { 0 };

static void * module_lookup(void * module) {
  unsigned int regionx;
  s_overlay *ovl = 0;
  usb_printf("module lookup(%p)\n", module);
  for (unsigned int i = 0; i < N_OVLY; i++) {
    if (overlays[i].start == module) {
      regionx = overlays[i].regionx;
      ovl = &overlays[i];
      break;
    }
  }
  if (!ovl) return 0;
  if (!cur_overlay[regionx] || cur_overlay[regionx] != ovl) {
    if (ovl->size == 0) {
      ovl->size = (char *)ovl->stop - (char *)ovl->start;
    }
    cur_overlay[regionx] = ovl;
    w25_read_data(SPI2, (uintptr_t)ovl->start, (unsigned int *)ovl->vma, (unsigned)ovl->size);
  }
  return ovl->func;
}

void led_on(void) {
  gpio_clear(GPIOE, GPIO5);
  usb_printf("LED ON\n");
}

void led_off(void) {
  gpio_set(GPIOE, GPIO5);
  usb_printf("LED OFF\n");
}

void led_on_stub(void) {
  void (*led_onp)(void) = module_lookup(&__load_start_led_on);
  return led_onp();
}

void led_off_stub(void) {
  void (*led_offp)(void) = module_lookup(&__load_start_led_off);
  return led_offp();
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

static void control_task(void *args __attribute__((unused))) {
  char ch;
  ch = usb_getc();
  switch (ch) {
    case '1':
      led_on_stub();
      usb_printf("LED ON\n");
      break;
    case '0':
      led_off_stub();
      usb_printf("LED OFF\n");
      break;
    default:
      usb_printf("error command\n");
      break;
  }
}


int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    usb_start();
    led_setup();
    spi_setup();
    xTaskCreate(led_task, "led", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, NULL);
    vTaskStartScheduler();
    for(;;);
    return 0;
}
