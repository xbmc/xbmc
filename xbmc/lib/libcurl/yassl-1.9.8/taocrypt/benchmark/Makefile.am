INCLUDES = -I../include -I../mySTL
bin_PROGRAMS       = benchmark
benchmark_SOURCES  = benchmark.cpp
benchmark_LDFLAGS  = -L../src
benchmark_LDADD        = ../src/libtaocrypt.la
benchmark_DEPENDENCIES = ../src/libtaocrypt.la
EXTRA_DIST = benchmark.dsp rsa1024.der dh1024.der dsa1024.der make.bat \
             benchmark.vcproj
