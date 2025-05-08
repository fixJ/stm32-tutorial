#ifndef IIC_H
#define IIC_H

#include <libopencm3/stm32/i2c.h>
#include <setjmp.h>

typedef enum {
  I2C_OK = 0,
  I2C_Addr_Timeout,
  I2C_Addr_NAK,
  I2C_Write_Timeout,
  I2C_Read_Timeout,
} I2C_Status;

enum I2C_RW {
  Read = 1,

  Write = 0
};

typedef struct {
  uint32_t device;
  uint32_t timeout;
} I2C_Control;

extern jmp_buf i2c_exception;

const char * i2c_error(I2C_Status status);

void i2c_configure(I2C_Control *dev, uint32_t i2c, uint32_t ticks);
void i2c_wait_busy(I2C_Control *dev);
void i2c_start_addr(I2C_Control *dev, uint8_t addr, enum I2C_RW rw);
void i2c_write(I2C_Control *dev, uint8_t byte);
void i2c_write_restart(I2C_Control *dev, uint8_t byte, uint8_t addr);
uint8_t i2c_read(I2C_Control *dev, bool lastf);

inline void i2c_stop(I2C_Control *dev) {
  i2c_send_stop(dev->device);
}


#endif //IIC_H
