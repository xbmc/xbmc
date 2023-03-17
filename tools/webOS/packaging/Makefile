include ../../depends/Makefile.include

APP_BUILD_DIR:=$(shell cd $(PREFIX)/../..; pwd)
APP_PACKAGE_DIR:=$(shell cd $(PREFIX)/../../..; pwd)/ipk-build
APP_PACKAGE_LIB_DIR:=$(APP_PACKAGE_DIR)/$(notdir $(abspath $(PREFIX)/..))/$(notdir $(abspath $(PREFIX)))/lib
APP_PACKAGE_LIBEXEC_DIR:=$(APP_PACKAGE_DIR)/$(notdir $(abspath $(PREFIX)/..))/$(notdir $(abspath $(PREFIX)))/libexec
APP_LIB_DIR:=$(shell cd $(PREFIX)/lib; pwd)/
APP_LIBEXEC_DIR:=$(shell cd $(PREFIX)/libexec; pwd)/
APP_IPK_DIR:=$(shell cd $(PREFIX)/../../..; pwd)
APP_PACKAGE:=$(shell awk '/APP_PACKAGE/ {print tolower($$2)}' $(VERSION.TXT))
VERSION_CODE:=$(shell awk '/VERSION_CODE/ {print tolower($$2)}' $(VERSION.TXT))
COMPANY_NAME:=$(shell awk '/COMPANY_NAME/ {print tolower($$2)}' $(VERSION.TXT))
IPK:=$(APP_PACKAGE)_$(VERSION_CODE)_all.ipk
ICON80:=icon.png
ICON130:=largeIcon.png
KEYBOARD:=keyboard.xml
MAIN_APP:=kodi-$(TARGET_PLATFORM)

COMPACT_IPK=compact-$(IPK)
COMPACT_PACKAGE_DIR=$(shell cd $(PREFIX)/../../..; pwd)/compact-build

ifneq ($(SDK_PATH),)
	LIBSTDC_DIR:=$(shell cd $(SDK_PATH)/../; pwd)/target/usr/lib
	LIBCRYPT_DIR:=$(shell cd $(SDK_PATH)/../; pwd)/target/lib
endif

.PHONY: all
all: clean libs $(IPK)

ipk: clean libs $(IPK)
compact-ipk: clean compact-clean $(COMPACT_IPK)

$(APP_PACKAGE_DIR):
	mkdir -p $(APP_PACKAGE_DIR)

$(APP_PACKAGE_DIR)/appinfo.json: appinfo.json.in Makefile | $(APP_PACKAGE_DIR)
	sed -e 's|@APP_PACKAGE@|$(APP_PACKAGE)|g' \
      -e 's|@VERSION_CODE@|$(VERSION_CODE)|g' \
      -e 's|@COMPANY_NAME@|$(COMPANY_NAME)|g' \
      -e 's|@APP_NAME@|$(APP_NAME)|g' < '$<' > '$@'

$(APP_PACKAGE_DIR)/kodi.sh: kodi.sh.in Makefile | $(APP_DIR)
	sed -e 's|@PREFIX@|$(PREFIX)|g' \
      -e 's|@MAIN_APP@|$(MAIN_APP)|g' < '$<' > '$@'
	chmod +x $(APP_PACKAGE_DIR)/kodi.sh

$(APP_PACKAGE_DIR)/$(ICON80): $(ICON80) | $(APP_PACKAGE_DIR)
	cp -t '$(APP_PACKAGE_DIR)' -- '$<'

$(APP_PACKAGE_DIR)/$(ICON130): $(ICON130) | $(APP_PACKAGE_DIR)
	cp -t '$(APP_PACKAGE_DIR)' -- '$<'

$(APP_PACKAGE_DIR)/.kodi/userdata/keymaps/$(KEYBOARD): $(KEYBOARD) | $(APP_PACKAGE_DIR)
	mkdir -p $(APP_PACKAGE_DIR)/.kodi/userdata/keymaps
	cp -t '$(APP_PACKAGE_DIR)/.kodi/userdata/keymaps' -- '$<'

$(IPK): $(APP_PACKAGE_DIR)/.kodi/userdata/keymaps/$(KEYBOARD) \
        $(APP_PACKAGE_DIR)/appinfo.json $(APP_PACKAGE_DIR)/kodi.sh \
        $(APP_PACKAGE_DIR)/$(ICON80) $(APP_PACKAGE_DIR)/$(ICON130) | $(APP_PACKAGE_DIR)
				cp $(APP_BUILD_DIR)/$(MAIN_APP) $(APP_PACKAGE_DIR)
				cp -R $(APP_BUILD_DIR)/addons $(APP_PACKAGE_DIR)
				cp -R $(APP_BUILD_DIR)/media $(APP_PACKAGE_DIR)
				cp -R $(APP_BUILD_DIR)/system $(APP_PACKAGE_DIR)
				cp -R $(APP_BUILD_DIR)/userdata $(APP_PACKAGE_DIR)
				ares-package \
				-o '$(APP_IPK_DIR)' \
				'$(APP_PACKAGE_DIR)'

$(COMPACT_IPK): $(APP_PACKAGE_DIR)/.kodi/userdata/keymaps/$(KEYBOARD) \
        $(APP_PACKAGE_DIR)/appinfo.json $(APP_PACKAGE_DIR)/kodi.sh \
        $(APP_PACKAGE_DIR)/$(ICON80) $(APP_PACKAGE_DIR)/$(ICON130) | $(APP_PACKAGE_DIR)
	mkdir -p $(COMPACT_PACKAGE_DIR)
	mkdir -p $(COMPACT_PACKAGE_DIR)/.kodi/userdata/keymaps
	cp $(APP_PACKAGE_DIR)/.kodi/userdata/keymaps/$(KEYBOARD) $(COMPACT_PACKAGE_DIR)/.kodi/userdata/keymaps/$(KEYBOARD)
	cp $(APP_PACKAGE_DIR)/appinfo.json $(COMPACT_PACKAGE_DIR)/appinfo.json
	cp $(APP_PACKAGE_DIR)/kodi.sh $(COMPACT_PACKAGE_DIR)/kodi.sh
	cp $(APP_PACKAGE_DIR)/$(ICON80) $(COMPACT_PACKAGE_DIR)/$(ICON80)
	cp $(APP_PACKAGE_DIR)/$(ICON130) $(COMPACT_PACKAGE_DIR)/$(ICON130)
	cp -R $(APP_BUILD_DIR)/addons $(COMPACT_PACKAGE_DIR)
	cp -R $(APP_BUILD_DIR)/media $(COMPACT_PACKAGE_DIR)
	cp -R $(APP_BUILD_DIR)/system $(COMPACT_PACKAGE_DIR)
	cp -R $(APP_BUILD_DIR)/userdata $(COMPACT_PACKAGE_DIR)
	cp $(APP_BUILD_DIR)/$(MAIN_APP) $(COMPACT_PACKAGE_DIR)
	ares-package \
  -o '$(APP_IPK_DIR)' \
  '$(COMPACT_PACKAGE_DIR)'

.PHONY: libs
libs:
	mkdir -p $(APP_PACKAGE_LIB_DIR)
	cp -R $(APP_LIB_DIR)* $(APP_PACKAGE_LIB_DIR)
	mkdir -p $(APP_PACKAGE_LIBEXEC_DIR)
	cp -R $(APP_LIBEXEC_DIR)* $(APP_PACKAGE_LIBEXEC_DIR)
ifneq ($(LIBSTDC_DIR),)
	cp -P $(LIBSTDC_DIR)/libstdc++.so* $(APP_PACKAGE_LIB_DIR)
	rm $(APP_PACKAGE_LIB_DIR)/libstdc++.so.6.0.30-gdb.py
endif
ifneq ($(LIBCRYPT_DIR),)
	cp -P $(LIBCRYPT_DIR)/libcrypt* $(APP_PACKAGE_LIB_DIR)
endif
	rm -f $(APP_PACKAGE_LIB_DIR)/*.a
	rm -f $(APP_PACKAGE_LIB_DIR)/*.la
	rm -f $(APP_PACKAGE_LIB_DIR)/libEGL.so*
	rm -f $(APP_PACKAGE_LIB_DIR)/libGLESv2.so*
	rm -f $(APP_PACKAGE_LIB_DIR)/libasound.*
	rm -f $(APP_PACKAGE_LIB_DIR)/libwayland-client.so*
	rm -f $(APP_PACKAGE_LIB_DIR)/libwayland-server.so*
	rm -f $(APP_PACKAGE_LIB_DIR)/libwayland-egl.so*

.PHONY: install
install:
	ares-install '$(APP_IPK_DIR)/$(IPK)'

.PHONY: launch
launch:
	ares-launch '$(APP_PACKAGE)'

.PHONY: uninstall
uninstall:
	ares-install --remove '$(APP_PACKAGE)'

.PHONY: clean
clean:
	rm -f -- '$(IPK)'
	rm -rf -- '$(APP_PACKAGE_DIR)'

.PHONY: compact-clean
compact-clean:
	rm -rf -- '$(COMPACT_PACKAGE_DIR)'
