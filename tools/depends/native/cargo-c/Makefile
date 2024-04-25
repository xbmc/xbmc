include ../../Makefile.include CARGO-C-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile CARGO-C-VERSION ../../download-files.include

PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)

export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig

APP=$(PLATFORM)/target/release/$(APPNAME)

CARGO_ENV_VARS = RUSTUP_HOME=$(PREFIX)/.rustup \
	CARGO_HOME=$(PREFIX)/.cargo \
	OPENSSL_DIR=$(PREFIX)
CARGO = $(CARGO_ENV_VARS) $(PREFIX)/bin/cargo

CLEANUP_CMD = [ -e $(PREFIX)/bin/cargo ] \
	&& $(CARGO) uninstall cargo-c || true

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(APP): $(PLATFORM)
	$(CARGO) build --release --manifest-path $(PLATFORM)/Cargo.toml

.installed-$(PLATFORM): $(APP)
	$(CARGO) install --profile release --path $(PLATFORM)

	touch $@

clean:
	$(CLEANUP_CMD)
	rm -f .installed-$(PLATFORM)

distclean::
	$(CLEANUP_CMD)
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
