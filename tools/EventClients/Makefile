# Default prefix dir
prefix=/usr/local
installdir=$(prefix)
WII_EXTRA_OPTS=

all: wiimote install

wiimote:
	cd Clients/WiiRemote && \
	g++ CWIID_WiiRemote.cpp -lcwiid -DICON_PATH="\"$(installdir)/share/pixmaps/xbmc/\"" $(WII_EXTRA_OPTS) -o WiiRemote

j2me-remote:
	cd Clients/J2ME\ Client
	ant -f build.xml

install:
	mkdir -p $(prefix)/bin
	cp -a Clients/WiiRemote/WiiRemote $(prefix)/bin/xbmc-wiiremote
	cp -a Clients/J2ME\ Client/j2me_remote.py $(prefix)/bin/xbmc-j2meremote
	cp -a Clients/PS3\ BD\ Remote/ps3_remote.py $(prefix)/bin/xbmc-ps3remote
	#cp -a Clients/PS3\ Sixaxis\ Controller/ps3d.py $(prefix)/bin/xbmc-ps3d
	mkdir -p $(prefix)/lib/python2.5/site-packages/xbmc
	echo 'ICON_PATH="$(installdir)/share/pixmaps/xbmc/"' > $(prefix)/lib/python2.5/site-packages/xbmc/defs.py
	cp -a lib/python/* $(prefix)/lib/python2.5/site-packages/xbmc/
	cp -a Clients/PS3\ BD\ Remote/ps3_remote.py $(prefix)/lib/python2.5/site-packages/xbmc/
	mkdir -p $(prefix)/include/xbmc
	cp -a lib/c++/* $(prefix)/include/xbmc/
	mkdir -p $(prefix)/share/pixmaps/xbmc
	cp -a icons/* $(prefix)/share/pixmaps/xbmc/

clean:
	rm -f Clients/WiiRemote/WiiRemote
