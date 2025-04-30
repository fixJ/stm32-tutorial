BINARY		= main
SRCFILES	= main.c oled.c ugui.c i2c.c rtos/heap_4.c rtos/list.c rtos/port.c rtos/tasks.c rtos/queue.c rtos/opencm3.c
LDSCRIPT	= stm32f103zet6.ld


TGT_CFLAGS	    += -I./rtos -I.
TGT_CXXFLAGS	+= -I./rtos -I.

ugui.o:	CFLAGS += -Wno-parentheses

all: elf bin

include ../../Makefile.incl

