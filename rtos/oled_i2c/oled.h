#ifndef OLED_H
#define OLED_H
#include <stdint.h>
#include "i2c.h"
#define OLED_ADDRESS 0x7B
#define OLED_WRITE_CMD 0x00
#define OLED_WRITE_DATA 0x40
void oled_write_command(I2C_Control *dev, uint8_t command);
void oled_write_command2(I2C_Control *dev, uint8_t cm1, uint8_t cm2);
void oled_write_data(I2C_Control *dev, uint8_t data);
void oled_reset(void);
void oled_init(I2C_Control *dev);




#endif //OLED_H
