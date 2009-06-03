# Build the adplug-$(ARCH).so shared lib that's tracked in the XBMC SVN repo.

# Set your ARCH accordingly (i.e. 'make ARCH=$ARCH')
ARCH ?= i486-linux

SO=adplug-$(ARCH).so
SRCS=AdplugXBMC.cpp
OBJS+=$(filter %.o,$(SRCS:.cpp=.o))
CXXFLAGS=-Wall -Werror
INCLUDES=-Iadplug-2.1/src $(shell pkg-config --cflags libbinio)

$(SO): adplug-2.1/src/.libs/libadplug.a $(OBJS)
	$(CC) -fPIC -DPIC -shared -o $@ $(OBJS) \
		adplug-2.1/src/.libs/libadplug.a

adplug-2.1/src/.libs/libadplug.a: adplug-2.1/Makefile
	make -C adplug-2.1

adplug-2.1/Makefile:
	cd adplug-2.1 && ./configure --enable-debug

clean:
	[ ! -f adplug-2.1/Makefile ] || make -C adplug-2.1 clean
	rm -f $(OBJS) $(LIB) $(SLIB) $(CLEAN_FILES) $(PCH:.h=.h.gch) $(OBJS:.o=.P)

distclean:
	[ ! -f adplug-2.1/Makefile ] || make -C adplug-2.1 distclean

.cpp.o:
	@rm -f ${<:.cpp=.o}
	$(CXX) -MD -c $(CXXFLAGS) $(DEFINES) $(INCLUDES) $< -o ${<:.cpp=.o}
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
