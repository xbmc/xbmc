cmake_minimum_required(VERSION 3.28)
project(pkgconf)
include(ExternalProject)

# Variables required
# TARBALL_DIR = location to download tarballs
# KODI_MIRROR = mirror address
# KODI_SOURCE_DIR = xbmc source root
# NATIVEPREFIX = install prefix

set(pkgconf_INSTALL_PREFIX ${NATIVEPREFIX})

# Dependency path
set(MODULE_PATH "${KODI_SOURCE_DIR}/tools/depends/native/pkgconf")

set(PKGCONF_FILE "${MODULE_PATH}/PKGCONF-VERSION")

file(STRINGS ${PKGCONF_FILE} PKGCONF_LNAME REGEX "^[ \t]*LIBNAME=")
file(STRINGS ${PKGCONF_FILE} PKGCONF_VER REGEX "^[ \t]*VERSION=")
file(STRINGS ${PKGCONF_FILE} PKGCONF_ARCHIVE REGEX "^[ \t]*ARCHIVE=")
file(STRINGS ${PKGCONF_FILE} PKGCONF_BASE_URL REGEX "^[ \t]*BASE_URL=")
file(STRINGS ${PKGCONF_FILE} PKGCONF_FULL_URL REGEX "^[ \t]*FULL_URL=")

# Tarball Hash
file(STRINGS ${PKGCONF_FILE} PKGCONF_HASH_SHA512 REGEX "^[ \t]*SHA512=")

string(REGEX REPLACE ".*LIBNAME=([^ \t]*).*" "\\1" PKGCONF_LNAME "${PKGCONF_LNAME}")
string(REGEX REPLACE ".*VERSION=([^ \t]*).*" "\\1" PKGCONF_VER "${PKGCONF_VER}")
string(REGEX REPLACE ".*ARCHIVE=([^ \t]*).*" "\\1" PKGCONF_ARCHIVE "${PKGCONF_ARCHIVE}")
string(REGEX REPLACE ".*BASE_URL=([^ \t]*).*" "\\1" PKGCONF_BASE_URL "${PKGCONF_BASE_URL}")
string(REGEX REPLACE ".*FULL_URL=([^ \t]*).*" "\\1" PKGCONF_FULL_URL "${PKGCONF_FULL_URL}")

string(REGEX REPLACE "\\$\\(LIBNAME\\)" "${PKGCONF_LNAME}" PKGCONF_ARCHIVE "${PKGCONF_ARCHIVE}")
string(REGEX REPLACE "\\$\\(VERSION\\)" "${PKGCONF_VER}" PKGCONF_ARCHIVE "${PKGCONF_ARCHIVE}")

# FULL_URL substitutes the following VERSION, BASE_URL
# FULL_URL syntax: FULL_URL=$(BASE_URL)/archive/$(VERSION).tar.gz
string(REGEX REPLACE "\\$\\(BASE_URL\\)" "${PKGCONF_BASE_URL}" PKGCONF_FULL_URL "${PKGCONF_FULL_URL}")
string(REGEX REPLACE "\\$\\(VERSION\\)" "${PKGCONF_VER}" PKGCONF_FULL_URL "${PKGCONF_FULL_URL}")

if(NOT PKGCONF_BASE_URL)
  set(PKGCONF_BASE_URL "${KODI_MIRROR}/build-deps/sources")
endif()

find_package(Meson REQUIRED)
find_package(Ninja REQUIRED)

ExternalProject_Add(pkgconf_build
  URL               ${PKGCONF_BASE_URL}/${PKGCONF_ARCHIVE}
  URL_HASH          ${PKGCONF_HASH_SHA512}
  DOWNLOAD_DIR      ${TARBALL_DIR}
  DOWNLOAD_NAME     ${PKGCONF_ARCHIVE}
  INSTALL_DIR       ${pkgconf_INSTALL_PREFIX}
  CONFIGURE_COMMAND ${MESON_EXECUTABLE} setup ./build
                    --prefix=${pkgconf_INSTALL_PREFIX}
                    --libdir=lib
                    --bindir=bin
                    --buildtype=release
                    -Dtests=disabled
  BUILD_COMMAND     ${NINJA_EXECUTABLE} -C ./build
  INSTALL_COMMAND   ${NINJA_EXECUTABLE} -C ./build install
  BUILD_IN_SOURCE   1
)
