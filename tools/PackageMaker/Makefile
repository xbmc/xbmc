TARGET = ../../XBMC_for_Mac.dmg

all: $(TARGET)

include ../../Makefile.include

$(TARGET): dist/XBMC.dmg
	mv -f dist/XBMC.dmg $(TARGET)

ifeq ($(findstring D_DEBUG,$(CFLAGS)), D_DEBUG)
dist/XBMC.dmg:
	./dmgmaker.pl ../../build/Debug/XBMC.app
else
dist/XBMC.dmg:
	./dmgmaker.pl ../../build/Release/XBMC.app
endif

