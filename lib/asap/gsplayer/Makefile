CC = arm-wince-mingw32ce-gcc -s -O2 -Wall -o $@ -I.. -I../win32
WINDRES = arm-wince-mingw32ce-windres -o $@

COMMON_C = ../asap.c ../acpu.c ../apokeysnd.c
COMMON_H = ../asap.h ../asap_internal.h ../anylang.h ../players.h

all: gspasap.dll

include ../players/Makefile

gspasap.dll: gspasap.c $(COMMON_C) ../win32/gui.c gspasap-res.o $(COMMON_H) ../win32/gui.h
	$(CC) -shared -DGSPLAYER gspasap.c $(COMMON_C) ../win32/gui.c gspasap-res.o

gspasap-res.o: ../win32/gui.rc ../asap.h ../win32/gui.h
	$(WINDRES) -D_WIN32_WCE -DGSPLAYER ../win32/gui.rc

clean:
	rm -f gspasap.dll gspasap-res.o ../players.h ../players/*.obx

.DELETE_ON_ERROR:
