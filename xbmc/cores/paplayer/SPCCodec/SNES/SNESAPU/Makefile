CC=g++
AS=nasm
OBJS=SPC700.o APU.o DSP.o SNESAPU.o
CFLAGS=-D_LINUX -fPIC -I../../ -O2
ASFLAGS=-Xvc -DLINUX -O1 -f elf

DIRS=alac_decoder libfaad mp4ff

SNESAPU-i486-linux.so: $(OBJS)
	$(CC) -shared -o $@ $(OBJS) `cat ../../../../DllLoader/exports/wrapper.def` ../../../../DllLoader/exports/wrapper.o && cp $@ ../../../../../../system/players/paplayer/

clean:
	$(RM) $(OBJS) *.so

%o : %cpp
	$(CC) -c -o $@ $< $(CFLAGS)

%o : %Asm
	$(AS) $(ASFLAGS) -o $@ $<

