JRE = C:/Program Files/Java/jdk1.6.0_10/jre
WTK = C:/Program Files/Java/SE_SDK/WTK2

JAVAC = javac -source 1.2 -d .
#JAVAC = gcj -C -d .
JAR = jar
JAVADOC = javadoc
PP = cpp -C -P -I..
PERL = perl
ME_CLASSES = $(WTK)/lib/cldcapi11.jar;$(WTK)/lib/midpapi20.jar;$(WTK)/lib/jsr75.jar
PREVERIFY = "$(WTK)/bin/preverify1.1"
ME_EMU = "$(WTK)/bin/emulatorw" -gui

COMMON_C = ../asap.c ../acpu.c ../apokeysnd.c
COMMON_H = ../asap.h ../asap_internal.h ../anylang.h
PLAYERS_OBX_DIR = net/sf/asap

all: asap2wav.jar asap_applet.jar asap_midlet.jad

include ../players/Makefile

asap2wav.jar: asap2wav.MF ASAP2WAV.class net/sf/asap/ASAP.class $(PLAYERS_OBX)
	$(JAR) cfm $@ asap2wav.MF ASAP2WAV.class net/sf/asap

ASAP2WAV.class: ASAP2WAV.java net/sf/asap/ASAP.class $(PLAYERS_OBX)
	$(JAVAC) -classpath . ASAP2WAV.java

asap_applet.jar: ASAPApplet.class net/sf/asap/ASAP.class $(PLAYERS_OBX)
	$(JAR) cf $@ ASAPApplet.class net/sf/asap

ASAPApplet.class: ASAPApplet.java net/sf/asap/ASAP.class
	$(JAVAC) -classpath "$(JRE)/lib/plugin.jar;." ASAPApplet.java

emu: asap_midlet.jad
	$(ME_EMU) -Xdescriptor:asap_midlet.jad

asap_midlet.jad: asap_midlet.jar asap_midlet.MF
	$(PERL) mf2jad.pl asap_midlet

asap_midlet.jar: asap_midlet.MF preverified/ASAPMIDlet.class $(PLAYERS_OBX)
	$(JAR) cfm $@ asap_midlet.MF -C preverified . $(PLAYERS_OBX)

preverified/ASAPMIDlet.class: ASAPMIDlet.class net/sf/asap/ASAP.class net/sf/asap/ASAP_ModuleInfo.class
	$(PREVERIFY) -classpath "$(ME_CLASSES);." -d preverified ASAPMIDlet FileList ASAPInputStream net.sf.asap.ASAP net.sf.asap.ASAP_ModuleInfo net.sf.asap.ASAP_State net.sf.asap.PokeyState

ASAPMIDlet.class: ASAPMIDlet.java net/sf/asap/ASAP.class
	$(JAVAC) -classpath . -bootclasspath "$(ME_CLASSES)" ASAPMIDlet.java

net/sf/asap/ASAP.class: ASAP.java net/sf/asap/ASAP_ModuleInfo.class
	$(JAVAC) -classpath . ASAP.java

ASAP.java: ASAP.ppjava $(COMMON_C) $(COMMON_H)
	$(PP) ASAP.ppjava > ASAP.java

net/sf/asap/ASAP_ModuleInfo.class: ASAP_ModuleInfo.java
	$(JAVAC) -classpath . ASAP_ModuleInfo.java

doc: ASAP.java ASAP_ModuleInfo.java
	$(JAVADOC) -d doc ASAP.java ASAP_ModuleInfo.java

clean:
	rm -f asap2wav.jar asap_applet.jar asap_midlet.jar asap_midlet.jad *.class ASAP.java
	rm -rf net preverified doc

.DELETE_ON_ERROR:
