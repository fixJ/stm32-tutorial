#include "spi_flash.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "FreeRTOS.h"
#include "task.h"
#include "usbcdc.h"

static const char *cap[3] = {
    "W25X16",
    "W25X32",
    "W25X64"
    "W25X128"
};

uint8_t w25_read_sr1(uint32_t spi) {
    uint8_t sr1;
    spi_enable(spi);
    spi_xfer(spi, W25_CMD_READ_SR1);
    sr1 = spi_xfer(spi, DUMMY);
    spi_disable(spi);
    return sr1;
}

uint8_t w25_read_sr2(uint32_t spi) {
    uint8_t sr1;
    spi_enable(spi);
    spi_xfer(spi, W25_CMD_READ_SR2);
    sr1 = spi_xfer(spi, DUMMY);
    spi_disable(spi);
    return sr1;
}

void w25_wait(uint32_t spi) {
  while(w25_read_sr1(spi) & W25_SR1_BUSY) {
    taskYIELD();
  }
}

bool w25_is_wprotect(uint32_t spi) {
  w25_wait(spi);
  return !(w25_read_sr1(spi) & W25_SR1_WEL);
}

void w25_write_en(uint32_t spi, bool en) {
  w25_wait(spi);
  spi_enable(spi);
  spi_xfer(spi, en?W25_CMD_WRITE_EN:W25_CMD_WRITE_DI);
  spi_disable(spi);
  w25_wait(spi);
}

uint16_t w25_manuf_device(uint32_t spi) {
  uint16_t info;
  w25_wait(spi);
  spi_enable(spi);
  spi_xfer(spi, W25_CMD_MANUF_DEVICE);
  spi_xfer(spi, DUMMY);
  spi_xfer(spi, DUMMY);
  spi_xfer(spi, DUMMY);
  info = spi_xfer(spi, DUMMY) << 8;
  info |= spi_xfer(spi, DUMMY);
  spi_disable(spi);
  return info;
}

uint32_t w25_JEDEC_ID(uint32_t spi) {
  uint32_t info;
  w25_wait(spi);
  spi_enable(spi);
  spi_xfer(spi, W25_CMD_JEDEC_ID);
  info = spi_xfer(spi, DUMMY);
  info = (info << 8) | spi_xfer(spi, DUMMY);
  info = (info << 8) | spi_xfer(spi, DUMMY);
  spi_disable(spi);
  return info;
}

void w25_read_uid(uint32_t spi, void *buf, uint16_t bytes) {
  uint8_t *udata = (uint8_t *) buf;
  if (bytes>8) {
    bytes = 8;
  } else if (bytes<=0) {
    return ;
  }

  w25_wait(spi);
  spi_enable(spi);
  spi_xfer(spi, W25_CMD_READ_UID);
  for(uint8_t ux=0; ux<4; ux++) {
    spi_xfer(spi, DUMMY);
  }
  for (uint8_t ux=0; ux<bytes; ux++) {
    udata[ux] = spi_xfer(spi, DUMMY);
  }
  spi_disable(spi);
}

void w25_power(uint32_t spi, bool on) {
  if (!on) {
    w25_wait(spi);
  }
  spi_enable(spi);
  spi_xfer(spi, on?W25_CMD_PWR_ON:W25_CMD_PWR_OFF);
  spi_disable(spi);
}

bool w25_chip_erase(uint32_t spi) {
  if (w25_is_wprotect(spi)) {
    usb_puts("Not Erased! Chip is not write enabled.\n");
    return false;
  }
  spi_enable(spi);
  spi_xfer(spi, W25_CMD_CHIP_EASE);
  spi_disable(spi);
  usb_puts("Chip erasing.\n");
  if (!w25_is_wprotect(spi)) {
    usb_puts("Not Erased! Chip is not write enabled.\n");
    return false;
  }
  usb_puts("Chip erased.\n");
  return true;
}


uint32_t w25_read_data(uint32_t spi, uint32_t addr, void * data, uint32_t len){
  uint8_t *udata = (uint8_t *) data;
  w25_wait(spi);
  spi_enable(spi);
  spi_xfer(spi, W25_CMD_FAST_READ);
  spi_xfer(spi, addr>>16);
  spi_xfer(spi, (addr>>8)&0xFF);
  spi_xfer(spi, addr&0xFF);
  spi_xfer(spi, DUMMY);
  for(; len-->0; ++addr) {
    *udata++ = spi_xfer(spi, DUMMY);
  }
  spi_disable(spi);
  return addr;
}

unsigned w25_write_data(uint32_t spi, uint32_t addr, void * data, uint32_t len){
  uint8_t *udata = (uint8_t *) data;
  w25_write_en(spi, true);
  w25_wait(spi);
  if (w25_is_wprotect(spi)) {
    usb_puts("Not Writed! Chip is not write enabled.\n");
    return 0xffffffff;
  }
  while(len>0) {
    spi_enable(spi);
    spi_xfer(spi, W25_CMD_WRITE_DATA);
    spi_xfer(spi, addr>>16);
    spi_xfer(spi, (addr>>8)&0xFF);
    spi_xfer(spi, addr&0xFF);
    while(len>0) {
      spi_xfer(spi, *udata++);
      --len;
      if((++addr & 0xff) == 0x00) {
        break;
      }
    }
    spi_disable(spi);
    if (len>0) {
      w25_write_en(spi, true);
    }
  }
  return addr;
}

void w25_erase_block(uint32_t spi, uint32_t addr, uint32_t cmd) {
  const char * what;
  if(w25_is_wprotect(spi)) {
    usb_puts("Not Erased! Chip is not write enabled.\n");
    return;
  }
}


void spi_setup(void) {
  rcc_periph_clock_enable(RCC_SPI2);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO12|GPIO13|GPIO15);
  gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO14);
  rcc_periph_reset_pulse(RCC_SPI2);
  spi_init_master(
      SPI2,
      SPI_CR1_BAUDRATE_FPCLK_DIV_256,
      SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
      SPI_CR1_CPHA_CLK_TRANSITION_1,
      SPI_CR1_DFF_8BIT,
      SPI_CR1_MSBFIRST
  );
  spi_disable_software_slave_management(SPI2);
  spi_enable_ss_output(SPI2);
}
