#ifndef USBCDC_H
#define USBCDC_H

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void usb_start(void);
bool usb_ready(void);

void usb_putc(char c);
void usb_puts(const char *s);
void usb_write(const char *s, unsigned len);
int usb_vprintf(const char *fmt, va_list ap);
int usb_printf(const char *fmt, ...);
int usb_getc(void);
int usb_getline(char *s, unsigned maxbuf);

#endif //USBCDC_H



