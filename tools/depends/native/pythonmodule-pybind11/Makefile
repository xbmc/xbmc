include ../../Makefile.include PYTHONMODULE-PYBIND11-VERSION
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile PYTHONMODULE-PYBIND11-VERSION

export PIP_CACHE_DIR=$(TARBALLS_LOCATION)/pip-cache

artifact=$(NATIVEPREFIX)/lib/python${NATIVE_PYTHON_VERSION}/site-packages/$(LIBNAME)/__init__.py

all: .installed-$(PLATFORM)

$(artifact):
	PYTHONPATH="$(NATIVEPREFIX)/lib/python${NATIVE_PYTHON_VERSION}/site-packages" $(NATIVEPREFIX)/bin/python3 -m pip install --ignore-installed $(LIBNAME)==$(VERSION)

.installed-$(PLATFORM): $(artifact)
	touch $@

clean:
	rm -rf .installed-$(PLATFORM)

distclean:
	rm -rf .installed-$(PLATFORM)
	cd $(NATIVEPREFIX)/lib/python${NATIVE_PYTHON_VERSION}/site-packages; rm -rf pybind11*
