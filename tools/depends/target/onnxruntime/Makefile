-include ../../Makefile.include
include ONNXRUNTIME-VERSION
DEPS = Makefile ONNXRUNTIME-VERSION

# ONNX Runtime is typically distributed as pre-built binaries
# This Makefile serves as a placeholder for dependency management
# In production, users should install ONNX Runtime via:
#   - Package manager (apt, brew, etc.)
#   - Direct download from https://github.com/microsoft/onnxruntime/releases
#   - Building from source (advanced)

# Minimum required version
REQUIRED_VERSION=1.16.0

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
  endif
endif

.PHONY: .installed-native

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS)
	@echo "ONNX Runtime $(VERSION) is required for semantic embedding features"
	@echo "Please install ONNX Runtime $(REQUIRED_VERSION)+ from:"
	@echo "  - System package manager"
	@echo "  - https://github.com/microsoft/onnxruntime/releases"
	@echo ""
	@echo "For Ubuntu/Debian:"
	@echo "  Download and install the .deb package or extract to /usr/local"
	@echo ""
	@echo "For development, you can set ONNXRUNTIME_ROOT to the installation path"
	mkdir -p $(PLATFORM)

.installed-$(PLATFORM): $(PLATFORM)
	@echo "ONNX Runtime: Manual installation required"
	@echo "Set ONNXRUNTIME_ROOT environment variable if installed in non-standard location"
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
