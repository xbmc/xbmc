VERSION = 2.0.0

# gcc for most ports
ASAP_GCC = mingw32-gcc -s -O2 -Wall -o $@ -I. -I..
ASAP_GPP = mingw32-g++ -s -O2 -Wall -o $@ -I. -I..
WINDRES = windres -o $@
LIB_GCC = mingw32-gcc -O2 -Wall -I..
AR = ar rc

# Microsoft compiler for Windows Media Player and foobar2000
DSHOW_BASECLASSES_DIR = "C:/Program Files/Microsoft SDKs/Windows/v6.1/Samples/Multimedia/DirectShow/BaseClasses"
FOOBAR2000_SDK_DIR = ../../foobar2000_SDK
CL = cl -nologo -O2 -GL -GR- -GS- -W3 -wd4996 -DNDEBUG
LINKOPT = -link -release
MKLIB = lib -nologo -ltcg -out:$@
ASAP_CL = $(CL) -Fe$@ -I. -I..

# old Microsoft compiler for XBMC plugin
ASAP_CL71 = cl -nologo -O2 -GL -GR- -W3 -DNDEBUG -Fe$@ -I. -I..

# common tools
PERL = perl
RM = rm -f
CP = cp -f
UNIX2DOS = unix2dos
ASCIIDOC = asciidoc -o $@ -a localtime -a doctime
ASCIIDOC_POSTPROCESS = $(PERL) -pi.bak -e "s/527bbd;/c02020;/;END{unlink '$@.bak'}" $@
ASCIIDOC_VALIDATE = xmllint --valid --noout --nonet $@
SEVENZIP = 7z a -mx=9
GREP = grep
CANDLE = candle -nologo -o $@
LIGHT = light -nologo -o $@

COMMON_C = ../asap.c ../acpu.c ../apokeysnd.c
COMMON_H = ../asap.h ../asap_internal.h ../anylang.h ../players.h

all: mingw dsf foo asap.lib

win32: mingw dsf foo xbmc COPYING.txt

mingw: asap2wav.exe wasap.exe in_asap.dll gspasap.dll ASAP_Apollo.dll apokeysnd.dll asapscan.exe libasap.a

include ../players/Makefile

# ASAP2WAV

asap2wav.exe: ../asap2wav.c $(COMMON_C) $(COMMON_H)
	$(ASAP_GCC) ../asap2wav.c $(COMMON_C)

# WASAP

wasap.exe: wasap/wasap.c $(COMMON_C) gui.c wasap/wasap-res.o $(COMMON_H) gui.h
	$(ASAP_GCC) -Wl,-subsystem,windows -DWASAP wasap/wasap.c $(COMMON_C) gui.c wasap/wasap-res.o -lcomdlg32 -lwinmm

wasap/wasap-res.o: gui.rc ../asap.h gui.h wasap/wasap.ico wasap/play.ico wasap/stop.ico
	$(WINDRES) -DWASAP gui.rc

# Winamp

in_asap.dll: winamp/in_asap.c $(COMMON_C) gui.c winamp/in_asap-res.o $(COMMON_H) gui.h winamp/in2.h winamp/ipc_pe.h winamp/wa_ipc.h
	$(ASAP_GCC) -shared -Wl,-subsystem,windows -DWINAMP winamp/in_asap.c $(COMMON_C) gui.c winamp/in_asap-res.o -lcomdlg32

winamp/in_asap-res.o: gui.rc ../asap.h gui.h
	$(WINDRES) -DWINAMP gui.rc

# foobar2000

foo: foo_asap.dll

FOOBAR2000_RUNTIME = $(FOOBAR2000_SDK_DIR)/foobar2000/foobar2000_component_client/component_client.cpp foobar2000/foobar2000_SDK.lib foobar2000/pfc.lib $(FOOBAR2000_SDK_DIR)/foobar2000/shared/shared.lib

foo_asap.dll: foobar2000/foo_asap.cpp $(COMMON_C) foobar2000/foo_asap.res $(COMMON_H) gui.h $(FOOBAR2000_RUNTIME)
	$(ASAP_CL) -Fofoobar2000/ -DFOOBAR2000 -LD -DWIN32 -DUNICODE -EHsc -I$(FOOBAR2000_SDK_DIR) foobar2000/foo_asap.cpp $(COMMON_C) foobar2000/foo_asap.res $(FOOBAR2000_RUNTIME) user32.lib $(LINKOPT)

foobar2000/foobar2000_SDK.lib: foobar2000/abort_callback.obj foobar2000/audio_chunk.obj foobar2000/audio_chunk_channel_config.obj foobar2000/file_info.obj foobar2000/filesystem.obj foobar2000/guids.obj foobar2000/mem_block_container.obj foobar2000/preferences_page.obj foobar2000/replaygain_info.obj foobar2000/service.obj
	$(MKLIB) $^

foobar2000/%.obj: $(FOOBAR2000_SDK_DIR)/foobar2000/SDK/%.cpp
	$(CL) -c -Fo$@ -DWIN32 -DUNICODE -EHsc -I$(FOOBAR2000_SDK_DIR) $<

foobar2000/pfc.lib: foobar2000/cfg_var.obj foobar2000/guid.obj foobar2000/other.obj foobar2000/string.obj foobar2000/string_conv.obj foobar2000/utf8.obj
	$(MKLIB) $^

foobar2000/%.obj: $(FOOBAR2000_SDK_DIR)/pfc/%.cpp
	$(CL) -c -Fo$@ -DWIN32 -DUNICODE -EHsc $<

foobar2000/foo_asap.res: gui.rc ../asap.h gui.h
	$(WINDRES) -DFOOBAR2000 gui.rc

# GSPlayer

gspasap.dll: ../gsplayer/gspasap.c $(COMMON_C) gui.c gspasap-res.o $(COMMON_H) gui.h
	$(ASAP_GCC) -shared -Wl,-subsystem,windows -Wl,--kill-at -DGSPLAYER ../gsplayer/gspasap.c $(COMMON_C) gui.c gspasap-res.o

gspasap-res.o: gui.rc ../asap.h gui.h
	$(WINDRES) -DGSPLAYER gui.rc

# GSPlayer - Windows CE

wince:
	$(MAKE) -C ../gsplayer

# DirectShow

dsf: asap_dsf.dll

DSHOW_BASECLASSES = $(DSHOW_BASECLASSES_DIR)/amfilter.cpp $(DSHOW_BASECLASSES_DIR)/combase.cpp $(DSHOW_BASECLASSES_DIR)/dllentry.cpp $(DSHOW_BASECLASSES_DIR)/dllsetup.cpp $(DSHOW_BASECLASSES_DIR)/mtype.cpp $(DSHOW_BASECLASSES_DIR)/source.cpp $(DSHOW_BASECLASSES_DIR)/wxlist.cpp $(DSHOW_BASECLASSES_DIR)/wxutil.cpp

asap_dsf.dll: dshow/asap_dsf.cpp $(COMMON_C) dshow/asap_dsf.res dshow/asap_dsf.def $(COMMON_H)
	$(ASAP_CL) -Fodshow/ -DDSHOW -LD -I$(DSHOW_BASECLASSES_DIR) dshow/asap_dsf.cpp $(COMMON_C) $(DSHOW_BASECLASSES) dshow/asap_dsf.res dshow/asap_dsf.def advapi32.lib ole32.lib oleaut32.lib strmiids.lib user32.lib winmm.lib $(LINKOPT)

dshow/asap_dsf.res: gui.rc ../asap.h
	$(WINDRES) -DDSHOW gui.rc

# Apollo

ASAP_Apollo.dll: apollo/ASAP_Apollo.cpp $(COMMON_C) gui.c ASAP_Apollo-res.o $(COMMON_H) gui.h
	$(ASAP_GPP) -shared -Wl,-subsystem,windows -DAPOLLO apollo/ASAP_Apollo.cpp $(COMMON_C) gui.c ASAP_Apollo-res.o -lcomdlg32

ASAP_Apollo-res.o: gui.rc ../asap.h gui.h
	$(WINDRES) -DAPOLLO gui.rc

# XBMC

xbmc: xbmc_asap.dll

xbmc_asap.dll: ../xbmc/xbmc_asap.c $(COMMON_C) ../xbmc/xbmc_asap.res $(COMMON_H)
	$(ASAP_CL71) -Fo../xbmc/ -LD -MD ../xbmc/xbmc_asap.c $(COMMON_C) ../xbmc/xbmc_asap.res $(LINKOPT)

../xbmc/xbmc_asap.res: gui.rc ../asap.h
	$(WINDRES) -DXBMC gui.rc

# APokeySound

apokeysnd.dll: ../apokeysnd.c apokeysnd-res.o ../asap.h ../asap_internal.h ../anylang.h
	$(ASAP_GCC) -shared -Wl,-subsystem,windows -DAPOKEYSND ../apokeysnd.c apokeysnd-res.o

apokeysnd-res.o: gui.rc ../asap.h
	$(WINDRES) -DAPOKEYSND gui.rc

# asapscan

asapscan.exe: ../asapscan.c $(COMMON_C) $(COMMON_H)
	$(ASAP_GCC) -DASAPSCAN ../asapscan.c $(COMMON_C)

# library

asap.lib: $(COMMON_C) $(COMMON_H)
	$(CL) -c $(COMMON_C)
	$(MKLIB) asap.obj acpu.obj apokeysnd.obj

libasap.a: $(COMMON_C) $(COMMON_H)
	$(LIB_GCC) -c $(COMMON_C)
	$(AR) $@ asap.o acpu.o apokeysnd.o

# Java

java:
	$(MAKE) -C ../java

# C#

csharp:
	$(MAKE) -C ../csharp

# JavaScript

javascript:
	$(MAKE) -C ../javascript

# Flash

flash:
	$(MAKE) -C ../flash

# delete generated files

clean:
	$(RM) *.txt *.html *.dll *.exe *.obj *.exp *.lib *.res *.o
	$(RM) setup/asap.wixobj
	$(RM) libasap.a
	$(RM) wasap/wasap-res.o wasap.exe
	$(RM) winamp/in_asap-res.o in_asap.dll
	$(RM) foobar2000/*.obj foobar2000/*.lib foobar2000/foo_asap.res
	$(RM) dshow/*.obj dshow/asap_dsf.res
	$(RM) ../xbmc/*.obj ../xbmc/xbmc_asap.res
	$(RM) install_dsf.bat uninstall_dsf.bat
	$(RM) ../players/*.obx ../players.h ../README.html
	$(MAKE) -C ../csharp clean
	$(MAKE) -C ../flash clean
	$(MAKE) -C ../gsplayer clean
	$(MAKE) -C ../java clean
	$(MAKE) -C ../javascript clean

# prepare files for release

COPYING.txt: ../COPYING
	$(UNIX2DOS) <$< >$@

../README.html: ../README ../NEWS ../CREDITS ../INSTALL
	$(ASCIIDOC) -a asapsrc -a asapports ../README
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

README_Flash.html: ../README ../NEWS ../CREDITS ../flash/USAGE
	$(ASCIIDOC) -a asapflash="(included in this binary package)" ../README
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

README_Java.html: ../README ../NEWS ../CREDITS ../java/USAGE
	$(ASCIIDOC) -a asapjava="(included in this binary package)" ../README
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

README_JavaScript.html: ../README ../NEWS ../CREDITS ../javascript/USAGE
	$(ASCIIDOC) -a asapjavascript="(included in this package)" ../README
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

README_Windows.html: ../README ../NEWS ../CREDITS USAGE
	$(ASCIIDOC) -a asapwin="(included in this binary package)" ../README
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

README_WindowsSetup.html: ../README ../NEWS ../CREDITS USAGE
	$(ASCIIDOC) -a asapwin="(included in this setup)" -a asapsetup ../README
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

README_WindowsCE.html: ../README ../NEWS ../CREDITS ../gsplayer/USAGE
	$(ASCIIDOC) -a asapwince="(included in this binary package)" ../README
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

install_dsf.bat:
	echo regsvr32 asap_dsf.dll>$@

uninstall_dsf.bat:
	echo regsvr32 /u asap_dsf.dll>$@

setup: ../../asap-$(VERSION)-win32.msi

../../asap-$(VERSION)-win32.msi: setup/asap.wixobj wasap/wasap.ico setup/license.rtf setup/asap-banner.jpg setup/asap-dialog.jpg setup/Website.url win32 README_WindowsSetup.html
	$(LIGHT) -ext WixUIExtension -sice:ICE69 $<

setup/asap.wixobj: setup/asap.wxs
	$(CANDLE) -dVERSION=$(VERSION) $<

srcdist: $(PLAYERS_OBX) ../README.html
	$(RM) ../../asap-$(VERSION).tar.gz
	$(PERL) -x.. ../maketar.pl -d asap-$(VERSION) \
		-t asap.c asap.h asap_internal.h anylang.h acpu.c apokeysnd.c \
		asap2wav.c asapscan.c \
		players/cmc.asx players/cms.asx players/dlt.as8 players/mpt.asx \
		players/rmt.asx players/tmc.asx players/tm2.asx \
		players/Makefile Makefile \
		ChangeLog COPYING CREDITS INSTALL NEWS README \
		README.html PORTS.xml PORTS.xsl \
		csharp/ASAP.ppcs csharp/asap2wav.cs csharp/Makefile \
		flash/ASAP.ppas flash/ASAP_ModuleInfo.as flash/PokeyState.as \
		flash/ASAPPlayer.as flash/Makefile flash/USAGE \
		gsplayer/gspasap.c gsplayer/mapplugin.h \
		gsplayer/Makefile gsplayer/USAGE \
		java/ASAP.ppjava java/ASAP_ModuleInfo.java \
		java/ASAP2WAV.java java/asap2wav.MF \
		java/ASAPApplet.java \
		java/ASAPMIDlet.java java/asap_midlet.MF java/mf2jad.pl \
		java/Makefile java/USAGE \
		javascript/asap.ppjs javascript/asap2wav.ppjs javascript/asapweb.ppjs \
		javascript/Makefile javascript/USAGE \
		moc/libasap_decoder.c \
		win32/Makefile win32/USAGE \
		win32/gui.c win32/gui.h win32/gui.rc \
		win32/apollo/ASAP_Apollo.cpp win32/apollo/InputPlugin.h \
		win32/dshow/asap_dsf.cpp win32/dshow/asap_dsf.def \
		win32/foobar2000/foo_asap.cpp \
		win32/setup/asap.wxs win32/setup/license.rtf \
		win32/setup/Website.url win32/setup/wix_mutex.pl \
		win32/wasap/wasap.c \
		win32/winamp/in_asap.c \
		win32/winamp/in2.h win32/winamp/out.h \
		win32/winamp/ipc_pe.h win32/winamp/wa_ipc.h \
		xbmc/xbmc_asap.c \
		xmms/libasap-xmms.c xmms/libasap-xmms.map \
		-b players/cmc.obx players/cm3.obx players/cms.obx players/dlt.obx players/mpt.obx \
		players/rmt4.obx players/rmt8.obx players/tmc.obx players/tm2.obx \
		win32/wasap/play.ico win32/wasap/stop.ico win32/wasap/wasap.ico \
		win32/setup/asap-banner.jpg win32/setup/asap-dialog.jpg \
		-s chksap.pl maketar.pl files2anylang.pl \
		| $(SEVENZIP) -tgzip -si ../../asap-$(VERSION).tar.gz

dist: srcdist win32 wince java csharp javascript flash setup \
	COPYING.txt README_Flash.html README_Java.html README_Windows.html README_WindowsCE.html \
	install_dsf.bat uninstall_dsf.bat
	$(RM) ../../asap-$(VERSION)-flash.zip
	$(RM) ../../asap-$(VERSION)-java.zip
	$(RM) ../../asap-$(VERSION)-win32.zip
	$(RM) ../../asap-$(VERSION)-wince-arm.zip
	$(SEVENZIP) -tzip ../../asap-$(VERSION)-flash.zip \
		COPYING.txt README_Flash.html ../flash/asap.swf
	$(SEVENZIP) -tzip ../../asap-$(VERSION)-java.zip \
		COPYING.txt README_Java.html \
		../java/asap2wav.jar ../java/asap_applet.jar ../java/asap_midlet.jad ../java/asap_midlet.jar
	$(SEVENZIP) -tzip ../../asap-$(VERSION)-win32.zip \
		COPYING.txt README_Windows.html \
		install_dsf.bat uninstall_dsf.bat \
		asap2wav.exe wasap.exe in_asap.dll foo_asap.dll gspasap.dll asap_dsf.dll ASAP_Apollo.dll apokeysnd.dll xbmc_asap.dll
	$(SEVENZIP) -tzip ../../asap-$(VERSION)-wince-arm.zip \
		COPYING.txt README_WindowsCE.html ../gsplayer/gspasap.dll
	$(GREP) -m 1 " released on " ../ChangeLog
	$(GREP) -m 1 ^ASAP ../NEWS
	$(GREP) ASAP_VERSION ../asap.h
	$(GREP) " Version =" ../csharp/ASAP.ppcs
	$(GREP) " VERSION =" ../java/ASAP.ppjava
	$(GREP) MIDlet-Version ../java/asap_midlet.MF
	$(GREP) ASAP_VERSION ../javascript/asap.ppjs
	$(GREP) VERSION ../flash/ASAP.ppas

www: ../../www/index.html ../../www/PORTS.xml ../../www/PORTS.xsl 

../../www/index.html: ../README ../NEWS ../CREDITS
	$(ASCIIDOC) -a asapwww -a asapports ../README
	$(ASCIIDOC_POSTPROCESS)
	$(ASCIIDOC_VALIDATE)

../../www/apokeysnd.dll: apokeysnd.dll
	$(CP) $< $@

../../www/asap.swf: ../flash/asap.swf
	$(CP) $< $@

../../www/asap_applet.jar: ../java/asap_applet.jar
	$(CP) $< $@

../../www/favicon.ico: wasap/wasap.ico
	$(CP) $< $@

../../www/PORTS.xml: ../PORTS.xml
	$(CP) $< $@

../../www/PORTS.xsl: ../PORTS.xsl
	$(CP) $< $@

.DELETE_ON_ERROR:
