#include "FreeRTOS.h"
#include "task.h"
#include "oled.h"
#include "usbcdc.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "i2c.h"
#include "ugui.h"


//static I2C_Control i2c_device;
//
//static void task(void * args __attribute__((unused))) {
//    i2c_configure(&i2c_device, I2C1, 10000);
//    //oled_init(&i2c_device);
//    oled_write_command(&i2c_device, 0xa5);
//    for(;;) {
//      vTaskDelay(pdMS_TO_TICKS(1000));
//    }
//}
//
//
//int main(void) {
//    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
//    rcc_periph_clock_enable(RCC_GPIOA);
//    rcc_periph_clock_enable(RCC_GPIOB);
//    rcc_periph_clock_enable(RCC_I2C1);
//    rcc_periph_clock_enable(RCC_AFIO);
//    gpio_set_mode(GPIOA,GPIO_MODE_OUTPUT_2_MHZ,
//              GPIO_CNF_OUTPUT_PUSHPULL,GPIO5);
//    gpio_set_mode(GPIOB,GPIO_MODE_OUTPUT_2_MHZ,
//              GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,GPIO6|GPIO7);
//    gpio_set(GPIOB,GPIO6|GPIO7);
//    gpio_set(GPIOA,GPIO5);
//    usb_start();
//    xTaskCreate(task,"task",200, NULL, configMAX_PRIORITIES-1, NULL);
//    vTaskStartScheduler();
//    for(;;);
//    return 0;
//}



#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"



void i2c_setup(void) {
    /* enable clock for GPIOB and I2C1 */
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_I2C1);

    /* configure pins PB6 (SCL) and PB7 (SDA) as Alternate Function */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO6 | GPIO7);

    /* reset and config I2C */


    i2c_peripheral_disable(SSD1306_I2C_PORT);

    i2c_set_clock_frequency(SSD1306_I2C_PORT, I2C_CR2_FREQ_36MHZ);
    i2c_set_fast_mode(SSD1306_I2C_PORT); /* fast mode */
    i2c_set_ccr(SSD1306_I2C_PORT, 180); /* configure CCR */
    i2c_set_trise(SSD1306_I2C_PORT, 37);

    i2c_peripheral_enable(SSD1306_I2C_PORT);
}


int main(){
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	i2c_setup();


	ssd1306_Init(); //send commands to configure ssd1306

	ssd1306_SetCursor(35,10); // set cursor x =   and y = 10
	ssd1306_WriteString("Hello", Font_11x18,White); // writes using font
	ssd1306_Line(5,32,123,32,White); //draw line


	ssd1306_UpdateScreen(); // do not forgot to update screen!




    for (;;);
	return 0;


}