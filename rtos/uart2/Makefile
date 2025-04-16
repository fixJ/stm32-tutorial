BINARY		= uart
SRCFILES	= uart.c rtos/heap_4.c rtos/list.c rtos/port.c rtos/tasks.c rtos/opencm3.c
LDSCRIPT	= stm32f103zet6.ld


TGT_CFLAGS	    += -I./rtos -I.
TGT_CXXFLAGS	+= -I./rtos -I.

all: elf bin

include ../../Makefile.incl

