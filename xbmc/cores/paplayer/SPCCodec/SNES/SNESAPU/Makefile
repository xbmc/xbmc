AS=nasm
OBJS=SPC700.o APU.o DSP.o SNESAPU.o
CFLAGS +=-D_LINUX -fPIC -I../../ -O2
ASFLAGS=-Xvc -DLINUX -O1 -f elf

LIB=../../../../../../system/players/paplayer/SNESAPU-i486-linux.so

include ../../../../../../Makefile.include

lib: $(LIB)

$(LIB): $(OBJS)
	$(CC) -shared -o $@ $(OBJS) `cat ../../../../DllLoader/exports/wrapper.def` ../../../../DllLoader/exports/wrapper.o

%o : %Asm
	$(AS) $(ASFLAGS) -o $@ $<

