include ../../Makefile.include LIBDOVI-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBDOVI-VERSION ../../download-files.include

LIBDYLIB=$(PLATFORM)/target/$(HOST)/release/$(BYPRODUCT)

CARGO_ENV_VARS = RUSTUP_HOME=$(NATIVEPREFIX)/.rustup \
	CARGO_HOME=$(NATIVEPREFIX)/.cargo
CARGO = $(CARGO_ENV_VARS) $(NATIVEPREFIX)/bin/cargo

CARGO_BASE_OPTS = --manifest-path $(PLATFORM)/dolby_vision/Cargo.toml
ifeq ($(CROSS_COMPILING),yes)
  ifeq ($(TARGET_PLATFORM),webos)
    RUST_TARGET = armv7-unknown-linux-gnueabi
  else
    RUST_TARGET = $(HOST)
  endif

  CARGO_BASE_OPTS += --target $(RUST_TARGET)
endif

CARGO_BUILD_OPTS = --offline \
	--frozen \
	--library-type staticlib \
	--profile release \
	--prefix $(PREFIX) \
	$(CARGO_BASE_OPTS)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM);
	$(CARGO) fetch $(CARGO_BASE_OPTS)

$(LIBDYLIB): $(PLATFORM)
	$(CARGO) cbuild $(CARGO_BUILD_OPTS)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(CARGO) cinstall $(CARGO_BUILD_OPTS)

	touch $@

clean:
	cd $(PLATFORM); $(CARGO) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
