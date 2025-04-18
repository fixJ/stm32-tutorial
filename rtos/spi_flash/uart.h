//
// Created by jxbeb on 2025/4/18.
//

#ifndef UART_H
#define UART_H
void uart_setup(void);
void uart_puts(char * s);
void uart_task(void *args __attribute((unused)));
#endif //UART_H
