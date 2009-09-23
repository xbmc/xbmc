## Process this file with automake to produce Makefile.in

include $(srcdir)/../Makefile.common

rs_skyrocket_SOURCES = $(COMMON) $(COLOR) $(VECTOR) $(IMAGE) $(SOUND) $(DLOPEN) \
	bee.cc bee.hh bigmama.cc bigmama.hh bomb.cc bomb.hh explosion.cc \
	explosion.hh flares.cc flares.hh fountain.cc fountain.hh meteor.cc \
	meteor.hh overlay.cc overlay.hh particle.hh resources.cc resources.hh \
	rocket.cc rocket.hh shockwave.cc shockwave.hh skyrocket.cc skyrocket.hh \
	smoke.cc smoke.hh spinner.cc spinner.hh star.cc star.hh streamer.cc \
	streamer.hh stretcher.cc stretcher.hh sucker.cc sucker.hh world.cc world.hh

AM_CPPFLAGS += $(LIBLTDL_CPPFLAGS) $(IMAGE_CPPFLAGS) $(SOUND_CPPFLAGS)
AM_CXXFLAGS += $(IMAGE_CXXFLAGS) $(SOUND_CXXFLAGS)
AM_LDFLAGS += $(IMAGE_LDFLAGS) $(SOUND_LDFLAGS)
rs_skyrocket_LDADD = $(LIBMISC) $(LIBLTDL) $(X_LIBS) $(OPENGL_LIBS) $(IMAGE_LIBS) $(SOUND_LIBS)

hack_PROGRAMS = rs-skyrocket

skyrocketdatadir = $(pkgdatadir)/skyrocket
dist_skyrocketdata_DATA = cloud.png earth-far.png earth-light.png \
	earth-near.png moon.png smoke1.png smoke2.png smoke3.png smoke4.png \
	smoke5.png
if SOUND
dist_skyrocketdata_DATA += boom1.ogg boom2.ogg boom3.ogg boom4.ogg \
	launch1.ogg launch2.ogg nuke.ogg popper.ogg suck.ogg whistle.ogg
endif SOUND

uninstall-hook:
	-rmdir $(skyrocketdatadir)
