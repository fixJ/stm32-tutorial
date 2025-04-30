#include "oled.h"
#include "FreeRTOS.h"
#include "task.h"
#include "i2c.h"
#include <libopencm3/stm32/gpio.h>

/*
* pin description
* GND -> GND
* VCC -> VCC
* CLK -> PB6
* MOS -> PB7
* RES -> PA5
* DC -> PA6 GND
* CS -> PA7 GND
 *
 * */

void oled_write_command(I2C_Control *dev, uint8_t command) {
  i2c_write(dev, command);
}

void oled_write_command2(I2C_Control *dev, uint8_t cmd1, uint8_t cmd2) {
  i2c_write(dev, cmd1);
  i2c_write(dev, cmd2);
}

void oled_write_data(I2C_Control *dev, uint8_t data) {
  i2c_write(dev, data);
}

void oled_reset(void) {
  gpio_clear(GPIOA, GPIO5);
  vTaskDelay(5);
  gpio_set(GPIOA, GPIO5);
}


void oled_init(I2C_Control *dev) {
  static uint8_t cmds[] = {
    0xAE, 0x00, 0x10, 0x40, 0x81, 0xCF, 0xA1, 0xA6,
    0xA8, 0x3F, 0xD3, 0x00, 0xD5, 0x80, 0xD9, 0xF1,
    0xDA, 0x12, 0xDB, 0x40, 0x8D, 0x14, 0xAF, 0xFF
  };
  oled_reset();
  i2c_start_addr(dev, OLED_ADDRESS, Write);
  i2c_write(dev, OLED_WRITE_CMD);
  for (uint8_t i = 0; cmds[i] != 0xff; i++) {
    oled_write_command(dev, cmds[i]);
  }
  i2c_stop(dev);
}