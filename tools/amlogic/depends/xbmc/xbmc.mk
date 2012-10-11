all: $(SOURCE)/xbmc.bin

include buildroot-sdk.mk
-include $(BUILDROOT)/package/python/python.mk
-include $(BUILDROOT)/package/thirdparty/xbmc/xbmc.mk
$(SOURCE)/xbmc.bin: $(SOURCE)/Makefile
	cd $(SOURCE); $(MAKE)

$(SOURCE)/Makefile: $(SOURCE)/configure
	cd $(SOURCE); $(TARGET_CONFIGURE_OPTS) $(TARGET_CONFIGURE_ARGS) $(XBMC_CONF_ENV) \
        ./configure --target=$(GNU_TARGET_NAME) --host=$(GNU_TARGET_NAME) --build=$(GNU_HOST_NAME) \
        --prefix=/usr --exec-prefix=/usr --sysconfdir=/etc $(XBMC_CONF_OPT) --disable-external-ffmpeg --enable-codec=amcodec

$(SOURCE)/configure: $(BUILDROOT)/package/thirdparty/xbmc/xbmc.mk  $(GCC)
	cd $(SOURCE); ./bootstrap

$(GCC):
	$(error gcc does not exist. Have you built Buildroot?)

$(BUILDROOT)/package/thirdparty/xbmc/xbmc.mk:
	$(error BUILDROOT must be defined in buildroot-sdk.mk)
