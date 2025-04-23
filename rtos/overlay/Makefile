BINARY		= main
SRCFILES	= main.c usbcdc.c spi_flash.c rtos/heap_4.c rtos/list.c rtos/port.c rtos/tasks.c rtos/queue.c rtos/opencm3.c
LDSCRIPT	= stm32f103zet6.ld




TGT_CFLAGS	    += -I./rtos -I.
TGT_CXXFLAGS	+= -I./rtos -I.

all: elf bin

include ../../Makefile.incl


#main.elf: $(OBJS)
#	$(LD) $(TGT_LDFLAGS) $(LDFLAGS) $(OBJS) $(LDLIBS) -o main.elf
#	@rm -f *.ov all.hex
#	for v in led_on led_off ; do \
#		$(OBJCOPY) -O ihex -j.$$v main.elf $$v.ov ; \
#		cat $$v.ov | sed '/^:04000005/d;/^:00000001/d' >>all.hex ; \
#	done
#	$(OBJCOPY) -Obinary -R.led_on -R.led_off main.elf main.bin
#
