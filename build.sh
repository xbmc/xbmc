#!/usr/bin/bash

. ./build_config.sh

# echo Build and install crossguid:

sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/crossguid PREFIX="${INSTALL_DIR}" CFLAGS="${CFLAGS_TOOLS}" CXXFLAGS="${CXXFLAGS_TOOLS}" CC="${C_COMPILER}" CXX="${CXX_COMPILER}"

# echo  Build and install flatbuffers:
# sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/flatbuffers PREFIX="${INSTALL_DIR}" CFLAGS="${CFLAGS_TOOLS}" CXXFLAGS="${CXXFLAGS_TOOLS}" CC="${C_COMPILER}" CXX="${CXX_COMPILER}"

echo Build and install libfmt:

# sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/libfmt PREFIX="${INSTALL_DIR}" CFLAGS="${CFLAGS_TOOLS}" CXXFLAGS="${CXXFLAGS_TOOLS}" CC="${C_COMPILER}" CXX="${CXX_COMPILER}"

#echo Build and install libspdlog:

#sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/libspdlog PREFIX="${INSTALL_DIR}" CFLAGS="${CFLAGS_TOOLS}" CXXFLAGS="${CXXFLAGS_TOOLS}" CC="${C_COMPILER}" CXX="${CXX_COMPILER}"

# echo Build and install wayland-protocols:

# sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/wayland-protocols PREFIX="${INSTALL_DIR}" CFLAGS="${CFLAGS_TOOLS}" CXXFLAGS="${CXXFLAGS_TOOLS}" CC="${C_COMPILER}" CXX="${CXX_COMPILER}"

# echo Build and install waylandpp:

#sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/waylandpp PREFIX="${INSTALL_DIR}"   CFLAGS="${CFLAGS_TOOLS}" CXXFLAGS="${CXXFLAGS_TOOLS} CC="${C_COMPILER}" CXX="${CXX_COMPILER}"
#WARNING: Building waylandpp has some dependencies of its own, namely scons, libwayland-dev (>= 1.11.0) and libwayland-egl1-mesa

# Build gtest

#sudo make -j$(getconf _NPROCESSORS_ONLN) -C tools/depends/target/gtest PREFIX="${INSTALL_DIR}"   CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" CC=/usr/bin/gcc CXX=/usr/bin/g++

mkdir $BUILD_DIR

echo  Change to build directory:

set -x
cd $BUILD_DIR

CORE_PLATFORM_NAMES="x11"
#CORE_PLATFORM_NAMES="wayland"
#CORE_PLATFORM_NAMES="x11 wayland"
RENDER_SYSTEM="gl"
#RENDER_SYSTEM="gles"
echo Configure build for ${CORE_PLATFORM_NAMES} and ${RENDER_SYSTEM}:

cmake -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
     -DCORE_PLATFORM_NAME="${CORE_PLATFORM_NAMES}" \
     -DAPP_RENDER_SYSTEM="${RENDER_SYSTEM}" \
     -DVERBOSE=1 \
     -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS[@]}" \
     -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS[@]}" \
     "${CMAKE_ARGS[@]}" \
     "${CMAKE_XX_ARGS[@]}" \
     "${TIDY}" \
      "${MAKE_CFLAGS_ARGS[@]}" \
      -DCMAKE_CXX_COMPILER="${CXX_COMPILER}" \
      -S "${KODI_SOURCE}" -B .


echo  Build Kodi

# Don't mess with this line. Build flags go on previous cmake lines that configure
# the project. This builds it.

cmake --build .  -- VERBOSE=1 -j$(getconf _NPROCESSORS_ONLN) #  --warn-unused-vars --check-system-vars --trace-expand

echo make install 

sudo make install DESTDIR=/  

sudo ln -sf "${BUILD_DIR}"/xbmc "${INSTALL_DIR}"/share/kodi/xbmc

echo 'to run a test: cd ${BUILD_DIR}; make check VERBOSE=1; kodi-test --gtest_filter=HttpHeader.*'

