#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usbcdc.h"
#include <miniprintf.h>
#include <getline.h>

static volatile bool initialized = false;

static QueueHandle_t usb_txq;
static QueueHandle_t usb_rxq;

static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_CDC,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x0483, //ST
    .idProduct = 0x5740, //virtual com port
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor comm_endp[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x83,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = 16,
        .bInterval = 255,
    }
};

static const struct usb_endpoint_descriptor data_endp[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x01,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = 64,
        .bInterval = 1,
    }, {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = 0x82,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = 64,
        .bInterval = 1,
    }
};

static const struct {
  struct usb_cdc_header_descriptor header;
  struct usb_cdc_call_management_descriptor call_mgmt;
  struct usb_cdc_acm_descriptor acm;
  struct usb_cdc_union_descriptor cdc_union;
} __attribute__((__packed__)) cdcacm_functional_descriptors = {
    .header = {
        .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        .bcdCDC = 0x0110,
    },
    .call_mgmt = {
        .bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        .bmCapabilities = 0,
        .bDataInterface = 1,

    },
    .acm = {
        .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        .bmCapabilities = 0,
    },
    .cdc_union = {
        .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_UNION,
        .bControlInterface = 0,
        .bSubordinateInterface0 = 1,
    }
};

static const struct usb_interface_descriptor comm_iface[] = {
    {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 1,
        .bInterfaceClass = USB_CLASS_CDC,
        .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
        .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
        .iInterface = 0,
        .endpoint = comm_endp,
        .extra = &cdcacm_functional_descriptors,
        .extralen = sizeof(cdcacm_functional_descriptors)
    }
};

static const struct usb_interface_descriptor data_iface[] = {
    {
        .bLength = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber = 1,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_CLASS_DATA,
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface = 0,
        .endpoint = data_endp,
    }
};

static const struct usb_interface ifaces[] = {
    {
        .num_altsetting = 1,
        .altsetting = comm_iface,
    },
    {
        .num_altsetting = 1,
        .altsetting = data_iface,
    }
};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80,
    .bMaxPower = 0x32,
    .interface = ifaces,
};

static const char * usb_strings[] = {
    "usbcdc.c driver",
    "usbcdc module",
    "usbcdc"
};

static uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes
cdcacm_control_request(
    usbd_device *usbd_dev __attribute__((unused)),
    struct usb_setup_data *req,
    uint8_t **buf __attribute__((unused)),
    uint16_t *len,
    void (**complete)(
        usbd_device *usbd_dev,
        struct usb_setup_data *req
    ) __attribute__((unused))
) {
  switch (req->bRequest) {
    case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
      return USBD_REQ_HANDLED;
    case USB_CDC_REQ_SET_LINE_CODING:
      if (*len < sizeof(struct usb_cdc_line_coding)) {
        return USBD_REQ_NOTSUPP;
      }
      return USBD_REQ_HANDLED;
  }
  return USBD_REQ_NOTSUPP;
}

static void
cdcacm_data_rx_cb(
    usbd_device *usbd_dev,
    uint8_t ep __attribute__((unused))
) {
  unsigned rx_avail = uxQueueSpacesAvailable(usb_rxq);
  char buff[64];
  int len, x;
  if (rx_avail <= 0) {
    return;
  }
  len = sizeof(buff) < rx_avail ? sizeof(buff) : rx_avail;
  len = usbd_ep_read_packet(usbd_dev, 0x01, buff, len);
  for (x=0; x<len; ++x) {
    xQueueSend(usb_rxq, &buff[x], 0);
  }
}

static void
cdcacm_set_config(
    usbd_device *usbd_dev,
    uint16_t wValue __attribute__((unused))
) {
  // endpoint1 receive
  usbd_ep_setup(
      usbd_dev,
      0x01,
      USB_ENDPOINT_ATTR_BULK,
      64,
      cdcacm_data_rx_cb
  );

  // endpoint2 send
  usbd_ep_setup(
      usbd_dev,
      0x82,
      USB_ENDPOINT_ATTR_BULK,
      64,
      NULL
  );
  usbd_register_control_callback(
      usbd_dev,
      USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
      USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
      cdcacm_control_request
  );
  initialized = true;
}


static void
usb_task(void *args) {
  usbd_device *udev = (usbd_device *) args;
  char txbuf[32];
  unsigned txlen = 0;
  for(;;) {
    usbd_poll(udev);
    if (initialized) {
      while(txlen < sizeof(txbuf) && xQueueReceive(usb_txq, &txbuf[txlen], portMAX_DELAY) == pdPASS) {
        ++txlen;
      }
      if (txlen > 0) {
        if (usbd_ep_write_packet(udev, 0x82, txbuf, txlen) != 0) {
          txlen = 0;
        }
      } else {
        taskYIELD();
      }
    }
  }
}


void usb_putc(char ch) {
  static const char cr = '\r';
  while (!usb_ready()) {
    taskYIELD();
  }
  if (ch == '\n') {
    xQueueSend(usb_txq, &cr, portMAX_DELAY);
  }
  xQueueSend(usb_txq, &ch, portMAX_DELAY);
}


void usb_puts(const char *s) {
  while (*s) {
    usb_putc(*s++);
  }
}

int usb_vprintf(const char *fmt, va_list ap) {
  return mini_vprintf_cooked(usb_putc, fmt, ap);
}

int usb_printf(const char *fmt, ...) {
  int rc;
  va_list args;
  va_start(args, fmt);
  rc = mini_vprintf_cooked(usb_putc, fmt, args);
  va_end(args);
  return rc;
}

void usb_write(const char *buf, unsigned len) {
  while (len-- > 0) {
    xQueueSend(usb_txq, buf, portMAX_DELAY);
    ++buf;
  }
}

int usb_getc(void) {
  char ch;
  uint32_t rc;
  rc = xQueueReceive(usb_rxq, &ch, portMAX_DELAY);
  if (rc != pdPASS) {
    return -1;
  }
  return ch;
}

int usb_getline(char * buf, unsigned bufsize) {
  return getline(buf, bufsize, usb_getc, usb_putc);
}

void usb_start(void) {
  usbd_device * udev = 0;
  usb_txq = xQueueCreate(128, sizeof(char));
  usb_rxq = xQueueCreate(128, sizeof(char));
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_USB);
  udev = usbd_init(&st_usbfs_v1_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
  usbd_register_set_config_callback(udev, cdcacm_set_config);
  xTaskCreate(usb_task, "usb_task", 200, udev, configMAX_PRIORITIES-1, NULL);
}

bool usb_ready(void) {
  return initialized;
}