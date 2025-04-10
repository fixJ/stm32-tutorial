BINARY		= main
SRCFILES	= main.c opencm3.c ../FreeRTOS-kernelV11.2.0/task.c ../FreeRTOS-kernelV11.2.0/portable/GCC/ARM_CM3/port.c
LDSCRIPT	= stm32f103zet6.ld


TGT_CFLAGS	    += -I../FreeRTOS-kernelV11.2.0/incude -I../FreeRTOS-kernelV11.2.0/portable/GCC/ARM_CM3/ -I.
TGT_CXXFLAGS	+= -I../FreeRTOS-kernelV11.2.0/incude -I../FreeRTOS-kernelV11.2.0/portable/GCC/ARM_CM3/ -I.

all: elf bin

include ../../Makefile.incl

