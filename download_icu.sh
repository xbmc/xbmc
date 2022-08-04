#!/usr/bin/bash
. ./build_config.sh
set -x
mkdir -p "${KODI_SOURCE}"/tools/depends/target/
cd  "${KODI_SOURCE}"/tools/depends/target/

curl -L https://github.com/unicode-org/icu/releases/download/release-71-1/icu4c-71_1-src.tgz | tar zxv

