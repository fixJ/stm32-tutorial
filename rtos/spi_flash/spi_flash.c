#include "spi_flash.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "FreeRTOS.h"
#include "task.h"
#include "usbcdc.h"
#include <ctype.h>
#include <intelhex.h>

static void spi_nss_enable(void) {
  gpio_clear(GPIOB, GPIO12);
}

static void spi_nss_disable(void) {
  gpio_set(GPIOB, GPIO12);
}

uint8_t w25_read_sr1(uint32_t spi) {
    uint8_t sr1;
    spi_enable(spi);
    spi_nss_enable();
    spi_xfer(spi, W25_CMD_READ_SR1);
    sr1 = spi_xfer(spi, DUMMY);
    spi_nss_disable();
    spi_disable(spi);
    return sr1;
}

uint8_t w25_read_sr2(uint32_t spi) {
    uint8_t sr2;
    spi_enable(spi);
    spi_nss_enable();
    spi_xfer(spi, W25_CMD_READ_SR2);
    sr2 = spi_xfer(spi, DUMMY);
    spi_nss_disable();
    spi_disable(spi);
    return sr2;
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
  spi_nss_enable();
  spi_xfer(spi, en?W25_CMD_WRITE_EN:W25_CMD_WRITE_DI);
  spi_nss_disable();
  spi_disable(spi);
  w25_wait(spi);
}

uint16_t w25_manuf_device(uint32_t spi) {
  uint16_t info;
  w25_wait(spi);
  spi_enable(spi);
  spi_nss_enable();
  spi_xfer(spi, W25_CMD_MANUF_DEVICE);
  spi_xfer(spi, DUMMY);
  spi_xfer(spi, DUMMY);
  spi_xfer(spi, DUMMY);
  info = spi_xfer(spi, DUMMY) << 8;
  info |= spi_xfer(spi, DUMMY);
  spi_nss_disable();
  spi_disable(spi);
  return info;
}

uint32_t w25_JEDEC_ID(uint32_t spi) {
  uint32_t info;
  w25_wait(spi);
  spi_enable(spi);
  spi_nss_enable();
  spi_xfer(spi, W25_CMD_JEDEC_ID);
  info = spi_xfer(spi, DUMMY);
  info = (info << 8) | spi_xfer(spi, DUMMY);
  info = (info << 8) | spi_xfer(spi, DUMMY);
  spi_nss_disable();
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
  spi_nss_enable();
  spi_xfer(spi, W25_CMD_READ_UID);
  for(uint8_t ux=0; ux<4; ux++) {
    spi_xfer(spi, DUMMY);
  }
  for (uint8_t ux=0; ux<bytes; ux++) {
    udata[ux] = spi_xfer(spi, DUMMY);
  }
  spi_nss_disable();
  spi_disable(spi);
}

void w25_power(uint32_t spi, bool on) {
  if (!on) {
    w25_wait(spi);
  }
  spi_enable(spi);
  spi_nss_enable();
  spi_xfer(spi, on?W25_CMD_PWR_ON:W25_CMD_PWR_OFF);
  spi_nss_disable();
  spi_disable(spi);
}

bool w25_chip_erase(uint32_t spi) {
  if (w25_is_wprotect(spi)) {
    usb_puts("Not Erased! Chip is not write enabled.\n");
    return false;
  }
  spi_enable(spi);
  spi_nss_enable();
  spi_xfer(spi, W25_CMD_CHIP_EASE);
  spi_nss_disable();
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
  spi_nss_enable();
  spi_xfer(spi, W25_CMD_FAST_READ);
  spi_xfer(spi, addr>>16);
  spi_xfer(spi, (addr>>8)&0xFF);
  spi_xfer(spi, addr&0xFF);
  spi_xfer(spi, DUMMY);
  for(; len-->0; ++addr) {
    *udata++ = spi_xfer(spi, DUMMY);
  }
  spi_nss_disable();
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
    spi_nss_enable();
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
    spi_nss_disable();
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
  //addr align
  switch (cmd) {
    case W25_CMD_ERA_SECTOR:
      what = "sector";
      addr &= ~(4*1024-1);
      break;
    case W25_CMD_ERA_32K:
      what = "32K block";
      addr &= ~(32*1024-1);
      break;
    case W25_CMD_ERA_64K:
      what = "64K block";
      addr &= ~(64*1024-1);
    default:
      return;
  }
  spi_enable(spi);
  spi_nss_enable();
  spi_xfer(spi, cmd);
  spi_xfer(spi, addr>>16);
  spi_xfer(spi, (addr>>8)&0xFF);
  spi_xfer(spi, addr&0xFF);
  spi_nss_disable();
  spi_disable(spi);
  usb_printf("%s erased, starting at %06X\n",
		what,(unsigned)addr);
}

void flash_status(void) {
  uint8_t s;
  s = w25_read_sr1(SPI2);
  usb_printf("SR1 = %02x (%s)\n", s, s & W25_SR1_WEL ? "write enabled" : "write disabled");
  usb_printf("SR2 = %02x\n", w25_read_sr2(SPI2));
}

unsigned int get_data24(const char * prompt) {
	unsigned int v = 0u, count = 0u;
    char ch;
    if (prompt) {
      usb_printf("%s", prompt);
    }
    while ((ch = usb_getc()) != '\r' && ch != '\n') {
      if (ch == '\b' || ch == 0x7f) {
        v >>= 4;
        usb_puts("\b \b");
        if (count > 0 ) {
          --count;
        }
        continue;
      }
      if (ch >= '0' && ch <= '9') {
        v <<= 4;
        v |= ch & 0x0F;
        usb_putc(ch);
      } else {
        if (isalpha(ch)) {
			ch = toupper(ch);
        }
        if (ch >= 'A' && ch <= 'Z') {
        	v <<= 4;
            v |= ((ch & 0x0F) - 1 + 10);
            usb_putc(ch);
        } else {
          usb_puts("??");
          continue;
        }
      }
      if (++count >= 6) {
        break;
      }
    }
	return v & 0xFFFFFF;
}

unsigned int get_data8(const char * prompt) {
	unsigned int v = 0u, count = 0u;
    char ch;
    if (prompt) {
      usb_printf("%s", prompt);
    }
    while ((ch = usb_getc()) != '\r' && ch != '\n' && !strchr(",./;\t", ch)) {
      if (ch == '"' || ch == '\'') {
        usb_putc(ch);
        v = usb_getc();
        usb_putc(v);
        count = 1;
        break;
      }
      if (ch == '\b' || ch == 0x7f) {
        v >>= 4;
        usb_puts("\b \b");
        if (count > 0 ) {
          --count;
        }
        continue;
      }
      if (ch >= '0' && ch <= '9') {
        v <<= 4;
        v |= ch & 0x0F;
        usb_putc(ch);
      } else {
        if (isalpha(ch)) {
			ch = toupper(ch);
        }
        if (ch >= 'A' && ch <= 'Z') {
        	v <<= 4;
            v |= ((ch & 0x0F) - 1 + 10);
            usb_putc(ch);
        } else {
          usb_puts("??");
          continue;
        }
      }
      if (++count >= 2) {
        break;
      }
    }
    if (!count) {
      return 0xffff;
    }
	return v & 0xFF;
}

uint32_t dump_page(uint32_t spi, uint32_t addr) {
  char buf[17];
  addr &= 0xff;
  for (unsigned int i = 0; i < 16; i++, addr+= 16) {
    usb_printf("%06x:", (unsigned int)addr);
    w25_read_data(spi, addr, buf, 16);
    for (uint32_t offset = 0; offset < 16; offset++) {
      usb_printf(" %02x", buf[offset]);
    }
    for (uint32_t offset = 0; offset < 16; offset++) {
      if (buf[offset] < ' ' || buf[offset] > 0x7f) {
        usb_putc('.');
      } else {
        usb_putc(buf[offset]);
      }
    }
    usb_putc('\n');
  }
  return addr;
}

void load_ihex(uint32_t spi) {
  s_ihex ihex;
  char buf[200], ch;
  unsigned int rtype, count=0, ux;
  if (w25_is_wprotect(spi)) {
    usb_printf("flash is write protected\n");
    return;
  }
  ihex_init(&ihex);
  usb_printf("ready for Intel hex upload:\n");
  for(;;){
    usb_printf("%08x ", (unsigned int)ihex.compaddr);
    while((ch = usb_getc()) != ':') {
      if (ch == 0x1a || ch == 0x04) {
        usb_printf("EOF\n");
        return;
      }
    }
    buf[0] = ch;
    usb_putc(ch);
    for (ux=1; ux+1<sizeof(buf); ++ux) {
      buf[ux] = ch = usb_getc();
      if (ch == '\r' || ch == '\n') {
        break;
      }
      if (ch == 0x1a || ch == 0x04) {
        usb_printf("EOF\n");
        return;
      }
      usb_putc(ch);
    }
    buf[ux]=0;
    if (!strchr(buf, ':')) {
      continue;
    }
    rtype = ihex_parse(&ihex, buf);
    switch (rtype) {
      case IHEX_RT_DATA:
        w25_write_data(spi, ihex.addr&0x00ffffff, ihex.data, ihex.length);
        ihex.compaddr += ihex.length;
        break;
      case IHEX_RT_EOF:
        break;
      case IHEX_RT_XSEG:
        break;
      case IHEX_RT_XLADDR:
        ihex.compaddr = ihex.baseaddr + ihex.addr;
        break;
      case IHEX_RT_SLADDR:
        break;
      default:
        usb_printf("Error %02x: '%s'\n", (unsigned int)rtype, buf);
        continue;
    }
    ++count;
    if (rtype == IHEX_RT_EOF) {
      break;
    }
    if (strchr(buf, 0x1a)||strchr(buf, 0x04)) {
      break;
    }

  }
}

//PB12-SS PB13-SCK PB14-MISO PB15-MOSI
void spi_setup(void) {
  rcc_periph_clock_enable(RCC_SPI2);
  rcc_periph_clock_enable(RCC_GPIOB);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13|GPIO15);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
  gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO14);
  rcc_periph_reset_pulse(RST_SPI2);
  spi_init_master(
      SPI2,
      SPI_CR1_BAUDRATE_FPCLK_DIV_256,
      SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
      SPI_CR1_CPHA_CLK_TRANSITION_1,
      SPI_CR1_DFF_8BIT,
      SPI_CR1_MSBFIRST
  );
}
