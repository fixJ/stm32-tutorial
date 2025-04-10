BINARY		= main
SRCFILES	= main.c opencm3.c rtos/tasks.c rtos/port.c
LDSCRIPT	= stm32f103zet6.ld


TGT_CFLAGS	    += -I./rtos -I.
TGT_CXXFLAGS	+= -I./rtos -I.

all: elf bin

include ../../Makefile.incl

