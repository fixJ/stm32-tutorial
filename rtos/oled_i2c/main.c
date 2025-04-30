#include "FreeRTOS.h"
#include "task.h"
#include "oled.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "i2c.h"
#include "ugui.h"

static I2C_Control i2c_device;

static void task(void * args __attribute__((unused))) {

}


int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);
    gpio_set_mode(GPIOA,GPIO_MODE_OUTPUT_2_MHZ,
              GPIO_CNF_OUTPUT_PUSHPULL,GPIO5);
    gpio_set_mode(GPIOB,GPIO_MODE_OUTPUT_2_MHZ,
              GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,GPIO6|GPIO7);
    gpio_set(GPIOB,GPIO6|GPIO7);
    i2c_configure(&i2c_device, I2C1, 1000);
    oled_init(&i2c_device);

    oled_command2(0x20,0x02);// Page mode
    oled_command(0x40);
    oled_command2(0xD3,0x00);
    for ( uint8_t px=0; px<8; ++px ) {
        oled_command(0xB0|px);
        oled_command(0x00); // Lo col
        oled_command(0x10); // Hi col
        for ( unsigned bx=0; bx<128; ++bx )
            oled_write_data(BMP1++);
    }

    for(;;);
    return 0;
}
