WII_EXTRA_OPTS=
CLEAN_FILES=Clients/WiiRemote/WiiRemote
PYTHON_SITEDIR := $(shell python -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")

all: install

wiimote: Clients/WiiRemote/WiiRemote

Clients/WiiRemote/WiiRemote:
	cd Clients/WiiRemote && \
	$(CXX) $(CXXFLAGS) $(LDFLAGS) CWIID_WiiRemote.cpp -lcwiid -DICON_PATH="\"$(datarootdir)/pixmaps/xbmc/\"" $(WII_EXTRA_OPTS) -o WiiRemote

j2me-remote:
	cd Clients/J2ME\ Client
	ant -f build.xml

install: Clients/WiiRemote/WiiRemote
	mkdir -p $(DESTDIR)$(bindir)
	cp -a Clients/WiiRemote/WiiRemote $(DESTDIR)$(bindir)/xbmc-wiiremote
	cp -a Clients/J2ME\ Client/j2me_remote.py $(DESTDIR)$(bindir)/xbmc-j2meremote
	cp -a Clients/PS3\ BD\ Remote/ps3_remote.py $(DESTDIR)$(bindir)/xbmc-ps3remote
	cp -a Clients/PS3\ Sixaxis\ Controller/ps3d.py $(DESTDIR)$(bindir)/xbmc-ps3d
	cp -a Clients/XBMC\ Send/xbmc-send.py $(DESTDIR)$(bindir)/xbmc-send
	mkdir -p $(DESTDIR)$(PYTHON_SITEDIR)/xbmc
	echo 'ICON_PATH="$(datarootdir)/pixmaps/xbmc/"' > $(DESTDIR)$(PYTHON_SITEDIR)/xbmc/defs.py
	cp -a lib/python/* $(DESTDIR)$(PYTHON_SITEDIR)/xbmc/
	cp -a Clients/PS3\ BD\ Remote/ps3_remote.py $(DESTDIR)$(PYTHON_SITEDIR)/xbmc/
	mkdir -p $(DESTDIR)$(includedir)/xbmc
	cp -a lib/c++/* $(DESTDIR)$(includedir)/xbmc/
	mkdir -p $(DESTDIR)$(datarootdir)/pixmaps/xbmc
	cp -a icons/* $(DESTDIR)$(datarootdir)/pixmaps/xbmc/

include ../../Makefile.include
