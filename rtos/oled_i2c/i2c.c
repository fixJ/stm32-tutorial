#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include <libopencm3/stm32/rcc.h>

#define systicks    xTaskGetTickCount


static const char * i2c_msg[] = {
    "OK",
    "Address timeout",
    "Address NAK",
    "Write timeout",
    "Read timeout"
};

jmp_buf i2c_exception;

static inline TickType_t diff_ticks(TickType_t early, TickType_t later) {
  if (later >= early) {
    return later - early;
  }
  return ~(TickType_t)0 - early + 1 + later;
}


const char * i2c_error(I2C_Status status) {
  int icode = (int)status;
  if (icode < 0 || icode > I2C_Read_Timeout) {
    return "Bad I2C status";
  }
  return i2c_msg[icode];
}

void i2c_configure(I2C_Control *dev, uint32_t i2c, uint32_t ticks) {
  dev->device = i2c,
  dev->timeout = ticks;

  i2c_peripheral_disable(dev->device);
  rcc_periph_reset_pulse(RST_I2C1);
  I2C_CR1(dev->device) &= ~I2C_CR1_STOP;
  i2c_set_standard_mode(dev->device);
  i2c_set_clock_frequency(dev->device, 36);
  i2c_set_trise(dev->device, 36);
  i2c_set_dutycycle(dev->device, I2C_CCR_DUTY_DIV2);
  i2c_set_ccr(dev->device, 180);
  i2c_set_own_7bit_slave_address(dev->device,0x23);
  i2c_peripheral_enable(dev->device);
}

void i2c_wait_busy(I2C_Control *dev) {
  while(I2C_SR2(dev->device) & I2C_SR2_BUSY) {
    taskYIELD();
  }
}

void i2c_start_addr(I2C_Control *dev, uint8_t addr, enum I2C_RW rw) {
  TickType_t t0 = systicks();
  i2c_wait_busy(dev);
  I2C_CR1(dev->device) &= ~I2C_SR1_AF;
  i2c_clear_stop(dev->device);
  if (rw == Read) {
    i2c_enable_ack(dev->device);
  }
  i2c_send_start(dev->device);
  //start bit is not sent or i2c is in slave mode and is in busy
  while(!((I2C_SR1(dev->device) & I2C_SR1_SB) && (I2C_SR2(dev->device) & (I2C_SR2_MSL|I2C_SR2_BUSY)))) {
    if (diff_ticks(t0, systicks()) > dev->timeout) {
      longjmp(i2c_exception, I2C_Addr_Timeout);
    }
    taskYIELD();
  }
  i2c_send_7bit_address(dev->device, addr, rw == Read ? I2C_READ: I2C_WRITE);
  t0 = systicks();
  while (!(I2C_SR1(dev->device) & I2C_SR1_ADDR)) {
    if(I2C_SR1(dev->device) & I2C_SR1_AF) {
      i2c_send_stop(dev->device);
      (void)I2C_SR1(dev->device);
      (void)I2C_SR2(dev->device);
      longjmp(i2c_exception, I2C_Addr_NAK);
    }
    if (diff_ticks(t0, systicks()) > dev->timeout) {
      longjmp(i2c_exception, I2C_Addr_Timeout);
    }
    taskYIELD();
  }
  (void)I2C_SR2(dev->device);
}

void i2c_write(I2C_Control *dev, uint8_t byte) {
  TickType_t t0 = systicks();
  i2c_send_data(dev->device, byte);
  while(!(I2C_SR1(dev->device) & I2C_SR1_BTF)) {
    if (diff_ticks(t0, systicks()) > dev->timeout) {
      longjmp(i2c_exception, I2C_Write_Timeout);
    }
    taskYIELD();
  }
}

uint8_t i2c_read(I2C_Control *dev, bool lastf) {
  TickType_t t0 = systicks();
  if (lastf) {
    i2c_disable_ack(dev->device);
  }
  // receive data register is empty, nothing can read
  while(!(I2C_SR1(dev->device) & I2C_SR1_RxNE)) {
    if (diff_ticks(t0, systicks()) > dev->timeout) {
      longjmp(i2c_exception, I2C_Read_Timeout);
    }
    taskYIELD();
  }
  // get data from data register
  return i2c_get_data(dev->device);
}

void i2c_write_restart(I2C_Control *dev, uint8_t byte, uint8_t addr) {
  TickType_t t0 = systicks();
  i2c_send_data(dev->device, byte);
  i2c_send_start(dev->device);
  taskENTER_CRITICAL();
  while(!(I2C_SR1(dev->device) & I2C_SR1_BTF)) {
    if (diff_ticks(t0, systicks()) > dev->timeout) {
      longjmp(i2c_exception, I2C_Write_Timeout);
    }
    taskYIELD();
  }
  t0 = systicks();
  while(!((I2C_SR1(dev->device) & I2C_SR1_SB) && (I2C_SR2(dev->device) & (I2C_SR2_MSL|I2C_SR2_BUSY)))) {
    if (diff_ticks(t0, systicks()) > dev->timeout) {
      longjmp(i2c_exception, I2C_Addr_Timeout);
    }
    taskYIELD();
  }
  i2c_send_7bit_address(dev->device, addr, I2C_READ);
  t0 = systicks();
  while (!(I2C_SR1(dev->device) & I2C_SR1_ADDR)) {
    if(I2C_SR1(dev->device) & I2C_SR1_AF) {
      i2c_send_stop(dev->device);
      (void)I2C_SR1(dev->device);
      (void)I2C_SR2(dev->device);
      longjmp(i2c_exception, I2C_Addr_NAK);
    }
    if (diff_ticks(t0, systicks()) > dev->timeout) {
      longjmp(i2c_exception, I2C_Addr_Timeout);
    }
    taskYIELD();
  }
  (void)I2C_SR2(dev->device);
}