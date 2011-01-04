# Angelscript makefile for linux (based on MingW makefile)
# Type 'make' then 'make install' to complete the installation of the library

# For 'make install' to work, set LOCAL according to your system configuration
LOCAL = /usr/local

# If you want to build a shared library, then run make with SHARED=1 and VERSION=version
ifdef SHARED
	LIB = libangelscript-$(VERSION).so
	DEVLIB = libangelscript.so
else
	LIB = libangelscript.a
endif
INC = angelscript.h

SRCDIR = ../../source
INCDIR = ../../include
ifeq ($(TARGETPLATFORM), iphone)
	IPHONEBIN =  /Developer/Platforms/iPhoneOS.platform/Developer/usr/bin
	OBJDIR = obj-iphone
	LIBDIR = ../../lib-iphone
	CXX = $(IPHONEBIN)/arm-apple-darwin9-g++-4.0.1 
	CXXFLAGS = -g -Wall -fPIC  -isysroot /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS2.2.sdk
else ifeq ($(TARGETPLATFORM), android)
	ANDROIDNDKROOT = /cygdrive/c/android/android-ndk-1.6_r1
	ANDROIDBIN = $(ANDROIDNDKROOT)/build/prebuilt/windows/arm-eabi-4.2.1/bin
	SYSROOT = $(ANDROIDNDKROOT)/build/platforms/android-4/arch-arm
	OBJDIR = obj-android
	LIBDIR = ../../lib-android
	CXX = $(ANDROIDBIN)/arm-eabi-gcc
	CXXFLAGS = -g -Wall -I$(SYSROOT)/usr/include \
	-Wall \
    -DANDROID \
	-fno-exceptions \
	-march=armv6 -mthumb-interwork \
	-mfloat-abi=softfp -fno-rtti
else
	OBJDIR = obj
	LIBDIR = ../../lib
	CXX = gcc
	# On i686 architecture you may need to add -march=i686 if you get 
	# an undefined symbol for __sync_sub_and_fetch_4 in as_atomic.cpp.
	CXXFLAGS = -g -Wall -fPIC
endif

DELETER = rm -f
COPIER = cp -a

SRCNAMES = \
  as_arrayobject.cpp \
  as_atomic.cpp \
  as_builder.cpp  \
  as_bytecode.cpp \
  as_callfunc.cpp \
  as_callfunc_arm.cpp \
  as_callfunc_mips.cpp \
  as_callfunc_ppc.cpp \
  as_callfunc_ppc_64.cpp \
  as_callfunc_sh4.cpp \
  as_callfunc_x86.cpp \
  as_callfunc_x64_gcc.cpp \
  as_compiler.cpp \
  as_context.cpp \
  as_configgroup.cpp \
  as_datatype.cpp \
  as_generic.cpp \
  as_gc.cpp \
  as_globalproperty.cpp \
  as_memory.cpp \
  as_module.cpp \
  as_objecttype.cpp \
  as_outputbuffer.cpp \
  as_parser.cpp \
  as_restore.cpp \
  as_scriptcode.cpp \
  as_scriptengine.cpp \
  as_scriptfunction.cpp \
  as_scriptnode.cpp \
  as_scriptobject.cpp \
  as_string.cpp \
  as_string_util.cpp \
  as_thread.cpp \
  as_tokenizer.cpp \
  as_typeinfo.cpp \
  as_variablescope.cpp \

OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRCNAMES:.cpp=.o)))
BIN = $(LIBDIR)/$(LIB)

ifeq ($(TARGETPLATFORM), iphone)
	OBJ += $(OBJDIR)/as_callfunc_arm_xcode.o
else ifeq ($(TARGETPLATFORM), android)
	OBJ += $(OBJDIR)/as_callfunc_arm_gcc.o
endif

all: $(BIN)

$(BIN): $(OBJDIR) $(LIBDIR) $(OBJ)
ifdef SHARED
	$(CXX) -shared -Wl,-soname,$(LIB) -o $(BIN) $(OBJ)
	( cd $(LIBDIR); ln -s $(LIB) $(DEVLIB) )
else
	ar r $(BIN) $(OBJ)
	ranlib $(BIN)
endif
	@echo -------------------------------------------------------------------
	@echo Done. As root, type 'make install' to install the library.

$(OBJDIR):
	mkdir $(OBJDIR)

$(LIBDIR):
	mkdir $(LIBDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: $(SRCDIR)/%.S
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: $(SRCDIR)/%.s
	$(CXX) $(CXXFLAGS) -o $@ -c $<


clean:
	$(DELETER) $(OBJ) $(BIN) $(LIBDIR)/$(DEVLIB)

install: $(BIN)
	@echo Installing to: $(LOCAL)/lib and $(LOCAL)/include...
	@echo -------------------------------------------------------------------
ifdef SHARED
	$(COPIER) $(LIBDIR)/$(DEVLIB) $(LOCAL)/lib
endif
	$(COPIER) $(BIN) $(LOCAL)/lib
	$(COPIER) $(INCDIR)/$(INC) $(LOCAL)/include
	@echo -------------------------------------------------------------------
	@echo Angelscript library installed. Enjoy!

uninstall:
	$(DELETER) $(LOCAL)/include/$(INC) $(LOCAL)/lib/$(LIB) $(LOCAL)/lib/$(DEVLIB)
	@echo -------------------------------------------------------------------
	@echo Angelscript library uninstalled.

.PHONY: all clean install uninstall
