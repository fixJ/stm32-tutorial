//
// Created by jxbeb on 2025/4/18.
//

#ifndef SPI_FLASH_H
#define SPI_FLASH_H

#include <stdint.h>
#include <stdbool.h>


#define W25_CMD_MANUF_DEVICE    0X90
#define W25_CMD_JEDEC_ID        0X9F
#define W25_CMD_WRITE_EN        0x06
#define W25_CMD_WRITE_DI        0x04
#define W25_CMD_READ_SR1        0x05
#define W25_CMD_READ_SR2        0x35
#define W25_CMD_CHIP_EASE       0xC7
#define W25_CMD_READ_DATA       0x03
#define W25_CMD_FAST_READ       0x0B
#define W25_CMD_WRITE_DATA      0x02
#define W25_CMD_READ_UID        0x4B
#define W25_CMD_PWR_ON          0xAB
#define W25_CMD_PWR_OFF         0xB9
#define W25_CMD_ERA_SECTOR      0x20
#define W25_CMD_ERA_32K         0x52
#define W25_CMD_ERA_64K         0xD8
#define DUMMY                   0x00
#define W25_SR1_BUSY            0X01
#define W25_SR1_WEL             0X02

uint8_t w25_read_sr1(uint32_t spi);
uint8_t w25_read_sr2(uint32_t spi);
void w25_wait(uint32_t spi);
bool w25_is_wprotected(uint32_t spi);
void w25_write_en(uint32_t spi, bool en);
uint16_t w25_manuf_device(uint32_t spi);
uint32_t w25_JEDEC_ID(uint32_t spi);
void w25_read_uid(uint32_t spi, void *buf, uint16_t bytes);
void w25_power(uint32_t spi, bool on);
bool w25_chip_erase(uint32_t spi);
uint32_t w25_read_data(uint32_t spi, uint32_t addr, void * data, uint32_t len);
unsigned w25_write_data(uint32_t spi, uint32_t addr, void * data, uint32_t len);
void w25_erase_block(uint32_t spi, uint32_t addr, uint32_t cmd);
void flash_status(void);
unsigned get_data24(const char * prompt);
unsigned get_data8(const char * prompt);
uint32_t dump_page(uint32_t spi, uint32_t addr);
void erase(uint32_t spi, uint32_t addr);
void load_ihex(uint32_t spi);
void monitor_task(void *args __attribute__((unused)));
void spi_setup(void);

#endif //SPI_FLASH_H
