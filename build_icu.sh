#!/usr/bin/bash

# NOTE: build_config.sh sets up ICU build and config options

. ./build_config.sh
set -x
mkdir -p "${BUILD_DIR}"/tools/depends/target/icu/source
cd "${BUILD_DIR}"/tools/depends/target/icu/source
"${ICU_CONFIG_ENVS[@]}" "${KODI_SOURCE}"/tools/depends/target/icu/source/runConfigureICU "${ICU_CONFIG_PLATFORM}" "${ICU_CONFIG_OPTS[@]}" --prefix="${INSTALL_DIR}" --includedir="${BUILD_DIR}"/build/include --libdir="${INSTALL_DIR}"/lib

gmake clean
gmake -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1 all
sudo gmake -j$(getconf _NPROCESSORS_ONLN) VERBOSE=1 install
