
include config

AR_FOR_BUILD ?= ar
CXX_FOR_BUILD ?= cxx
RANLIB_FOR_BUILD ?= ranlib

SRC = alpha.cpp clusterfit.cpp colourblock.cpp colourfit.cpp colourset.cpp maths.cpp rangefit.cpp singlecolourfit.cpp squish.cpp

OBJ = $(SRC:%.cpp=%.o)

LIB = libsquish.a

all : $(LIB) squish.pc

install : $(LIB) squish.pc
	install squish.h $(INSTALL_DIR)/include 
	install libsquish.a $(INSTALL_DIR)/lib
	install squish.pc $(INSTALL_DIR)/lib/pkgconfig

uninstall:
	$(RM) $(INSTALL_DIR)/include/squish.h
	$(RM) $(INSTALL_DIR)/lib/libsquish.a

$(LIB) : $(OBJ)
	$(AR_FOR_BUILD) cr $@ $?
	$(RANLIB_FOR_BUILD) $@

%.o : %.cpp
	$(CXX_FOR_BUILD) $(CPPFLAGS) -I. $(CXXFLAGS) -o$@ -c $<

clean :
	$(RM) $(OBJ) $(LIB)

squish.pc:
	sed 's|@PREFIX@|$(INSTALL_DIR)|' $@.in > $@

