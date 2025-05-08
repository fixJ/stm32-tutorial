BINARY		= main
SRCFILES	= main.c ssd1306.c ssd1306_fonts.c rtos/heap_4.c rtos/list.c rtos/port.c rtos/tasks.c rtos/queue.c rtos/opencm3.c
LDSCRIPT	= stm32f103zet6.ld


TGT_CFLAGS	    += -I./rtos -I.
TGT_CXXFLAGS	+= -I./rtos -I.

ugui.o:	CFLAGS += -Wno-parentheses

all: elf bin

include ../../Makefile.incl

