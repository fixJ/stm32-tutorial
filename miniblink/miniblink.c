#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void
gpio_setup(void) {

    /* Enable GPIOE clock. */
    rcc_periph_clock_enable(RCC_GPIOE);

    /* Set GPIO8 (in GPIO port C) to 'output push-pull'. */
    gpio_set_mode(GPIOE,GPIO_MODE_OUTPUT_2_MHZ,
              GPIO_CNF_OUTPUT_PUSHPULL,GPIO5);
}

int
main(void) {
    int i;

    gpio_setup();

    for (;;) {
        gpio_toggle(GPIOE, GPIO5);
        for (i = 0; i < 1500000; i++)	/* Wait a bit. */
            __asm__("nop");

        gpio_toggle(GPIOE, GPIO5);
        for (i = 0; i < 2000000; i++)	/* Wait a bit. */
            __asm__("nop");
    }

    return 0;
}