SRCDIR = $(top_srcdir)/source/portaudiocpp

lib_LTLIBRARIES = libportaudiocpp.la

LDADD = libportaudiocpp.la

libportaudiocpp_la_LDFLAGS = -version-info $(LT_VERSION_INFO) -no-undefined

libportaudiocpp_la_LIBADD = $(top_builddir)/$(PORTAUDIO_ROOT)/lib/libportaudio.la
libportaudiocpp_la_SOURCES = \
       $(SRCDIR)/BlockingStream.cxx \
       $(SRCDIR)/CallbackInterface.cxx \
       $(SRCDIR)/CallbackStream.cxx \
       $(SRCDIR)/CFunCallbackStream.cxx \
       $(SRCDIR)/CppFunCallbackStream.cxx \
       $(SRCDIR)/Device.cxx \
       $(SRCDIR)/DirectionSpecificStreamParameters.cxx \
       $(SRCDIR)/Exception.cxx \
       $(SRCDIR)/HostApi.cxx \
       $(SRCDIR)/InterfaceCallbackStream.cxx \
       $(SRCDIR)/MemFunCallbackStream.cxx \
       $(SRCDIR)/Stream.cxx \
       $(SRCDIR)/StreamParameters.cxx \
       $(SRCDIR)/System.cxx \
       $(SRCDIR)/SystemDeviceIterator.cxx \
       $(SRCDIR)/SystemHostApiIterator.cxx
