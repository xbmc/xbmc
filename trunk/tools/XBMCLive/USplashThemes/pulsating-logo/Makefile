CC=gcc
CFLAGS=-g -Wall -fPIC
LDFLAGS=
INCLUDES=
COMPILE = $(CC) $(INCLUDES) $(CFLAGS)
LINK = $(CC) $(CFLAGS) $(LDFLAGS)

xbmc-splash.so: xbmc_1920_1200.png.c.o xbmc_1920_1080.png.c.o xbmc_1680_1050.png.c.o xbmc_1440_900.png.c.o xbmc_1280_1024.png.c.o xbmc_1366_768.png.c.o xbmc_1280_720.png.c.o xbmc_1024_768.png.c.o xbmc_800_600.png.c.o xbmc_640_480.png.c.o xbmc_spinner.png.c.o xbmc-splash.c.o helvB10.bdf.c.o
	$(COMPILE) -shared -o $@ $^

%.c.o: %.c
	$(COMPILE) -o $@ -c $<

%.png.c: %.png
	pngtousplash $< > $@

%.bdf.c: %.bdf
	bdftousplash $< > $@

clean:
	rm -f *.png.c *.bdf.c *.c.o *.so

install:
	cp xbmc-splash.so /usr/lib/usplash
	rm /etc/alternatives/usplash-artwork.so
	ln -s /usr/lib/usplash/xbmc-splash.so /etc/alternatives/usplash-artwork.so
	dpkg-reconfigure usplash
test:
	usplash -c
