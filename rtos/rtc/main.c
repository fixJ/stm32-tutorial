#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "usbcdc.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/cortex.h>
#include <time.h>
#include <string.h>
#include <stdio.h>


static TaskHandle_t task1_handle, task2_handle, task3_handle;
static SemaphoreHandle_t h_mutex;

static volatile unsigned int
    rtc_isr_count = 0u,
    rtc_alarm_count = 0u,
    rtc_overflow_count = 0u;

static volatile unsigned int
    days = 0,
    hours = 0, minutes = 0, seconds = 0,
    alarm = 0;


static void mutex_lock(void) {
  xSemaphoreTake(h_mutex, portMAX_DELAY);
}

static void mutex_unlock(void) {
  xSemaphoreGive(h_mutex);
}

void rtc_isr(void) {
  UBaseType_t intstatus;
  BaseType_t woken = pdFALSE;
  ++rtc_isr_count;
  if (rtc_check_flag(RTC_OW)) {
    ++rtc_overflow_count;
    rtc_clear_flag(RTC_OW);
    if (!alarm) {
      rtc_clear_flag(RTC_ALR);
    }
    vTaskNotifyGiveFromISR(task1_handle, &woken);
    portYIELD_FROM_ISR(woken);
  }
  if (rtc_check_flag(RTC_SEC)) {
    rtc_clear_flag(RTC_SEC);
    intstatus = taskENTER_CRITICAL_FROM_ISR();
    if (++seconds >= 60) {
      ++minutes;
      seconds -= 60;
    }
    if (minutes >= 60) {
      ++hours;
      minutes -= 60;
    }
    if (hours >= 24) {
      ++days;
      hours -= 24;
    }
    taskEXIT_CRITICAL_FROM_ISR(intstatus);
    vTaskNotifyGiveFromISR(task2_handle, &woken);
    portYIELD_FROM_ISR(woken);
    return;
  }
  if (rtc_check_flag(RTC_ALR)) {
    ++rtc_alarm_count;
    rtc_clear_flag(RTC_ALR);
    vTaskNotifyGiveFromISR(task3_handle, &woken);
    portYIELD_FROM_ISR(woken);
    return;
  }
}

static void set_alarm(unsigned int seconds) {
  alarm = (rtc_get_counter_val() + seconds) & 0xffffffff;
  rtc_disable_alarm();
  rtc_set_alarm_time(rtc_get_counter_val() + seconds);
  rtc_enable_alarm();
}

static void alarm_task3(void * args __attribute__((unused))) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    mutex_lock();
    usb_printf("*** ALARM interrupt*** at %3u days %02u:%02u:%02u alarm count is %d\n",
      days,hours,minutes,seconds, rtc_alarm_count);
    mutex_unlock();
  }
}

static void sec_task2(void * args __attribute__((unused))) {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    mutex_lock();
    usb_printf("*** SEC interrupt*** at %3u days %02u:%02u:%02u\n",
      days,hours,minutes,seconds);
    mutex_unlock();
  }
}

static void overflow_task1(void * args __attribute__((unused))) {
  for(;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    mutex_lock();
    usb_printf("*** OVERFLOW interrupt*** overflow count is %d\n", rtc_overflow_count);
    mutex_unlock();
  }
}

static void rtc_setup(void) {
  rcc_enable_rtc_clock();
  rtc_interrupt_disable(RTC_SEC);
  rtc_interrupt_disable(RTC_ALR);
  rtc_interrupt_disable(RTC_OW);

  rtc_awake_from_off(RCC_HSE);
  rtc_set_prescaler_val(62500);
  rtc_set_counter_val(0xfffffff0);

  nvic_enable_irq(NVIC_RTC_IRQ);

  cm_disable_interrupts();
  rtc_clear_flag(RTC_SEC);
  rtc_clear_flag(RTC_ALR);
  rtc_clear_flag(RTC_OW);
  rtc_interrupt_enable(RTC_SEC);
  rtc_interrupt_enable(RTC_ALR);
  rtc_interrupt_enable(RTC_OW);
  cm_enable_interrupts();
}



int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    h_mutex = xSemaphoreCreateMutex();
    usb_start();
    xTaskCreate(overflow_task1, "task1", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, &task1_handle);
    xTaskCreate(sec_task2, "task2", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES-1, &xTaskHandle);
    xTaskCreate(alarm_task3, "task3", configMINIMAL_STACK_SIZE, NULL, 1, &xTaskHandle);
    vTaskStartScheduler();
    for(;;);
    return 0;
}
