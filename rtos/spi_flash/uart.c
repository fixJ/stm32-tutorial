#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

static QueueHandle_t uart_txq;
static QueueHandle_t uart_rxq;

void uart_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART1);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
    usart_set_baudrate(USART1, 115200);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_mode(USART1, USART_MODE_TX);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    uart_txq = xQueueCreate(256, sizeof(char));
    uart_rxq = xQueueCreate(256, sizeof(char));
    usart_enable(USART1);
}


void uart_puts(char * s) {
    for (;*s;++s) {
        xQueueSend(uart_txq, s, portMAX_DELAY);
    }
}


void uart_task(void *args __attribute((unused))) {
    char ch;
    for(;;){
        if (xQueueReceive(uart_txq, &ch, 1000) == pdPASS) {
            while(!usart_get_flag(USART1, USART_SR_TXE)){
                taskYIELD();
            }
            usart_send(USART1, ch);
        }
        gpio_toggle(GPIOE, GPIO5);
    }
}
