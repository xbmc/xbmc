#!/bin/bash

#      Copyright (C) 2015 Team MrMC
#      https://github.com/MrMC
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with MrMC; see the file COPYING.  If not, see
#  <http://www.gnu.org/licenses/>.

set -x

TARGET_CONTENTS="${TARGET_BUILD_DIR}/${EXECUTABLE_FOLDER_PATH}"
TARGET_FRAMEWORKS=$TARGET_BUILD_DIR/$FRAMEWORKS_FOLDER_PATH

# use the same date/time stamp format for all CFBundleVersions
BUNDLE_REVISION=$(date -u +%y%m%d.%H%M)

# ios/tvos use different framework plists
if [ "${PLATFORM_NAME}" == "appletvos" ]; then
  SEEDFRAMEWORKPLIST="${SRCROOT}/xbmc/platform/darwin/tvos/FrameworkSeed_Info.plist"
# todo: implement soft frameworks for ios
#elif [ "$PLATFORM_NAME" == "iphoneos" ]; then
#  SEEDFRAMEWORKPLIST="${SRCROOT}/xbmc/platform/darwin/ios/FrameworkSeed_Info.plist"
fi

function convert2framework
{
  DYLIB="${1}"
  # typical darwin dylib name format is lib<name>.<version>.dylib
  DYLIB_BASENAME=$(basename "${DYLIB}")
  # strip .<version>.dylib
  DYLIB_LIBBASENAME="${DYLIB_BASENAME%%.[0-9]*}"
  # make sure .dylib is stripped
  DYLIB_LIBNAME="${DYLIB_LIBBASENAME%.dylib}"

  # Update main bundle executable to new location of frameworks
  install_name_tool -change  @executable_path/Frameworks/${DYLIB_BASENAME} @executable_path/Frameworks/${DYLIB_LIBNAME}.framework/${DYLIB_LIBNAME} ${TARGET_BUILD_DIR}/${EXECUTABLE_FOLDER_PATH}/${EXECUTABLE_NAME}
  install_name_tool -add_rpath @executable_path/Frameworks/${DYLIB_LIBNAME}.framework ${TARGET_BUILD_DIR}/${EXECUTABLE_FOLDER_PATH}/${EXECUTABLE_NAME}

  BUNDLEID=$(/usr/libexec/PlistBuddy -c 'Print CFBundleIdentifier' ${TARGET_BUILD_DIR}/${EXECUTABLE_FOLDER_PATH}/Info.plist)
  FRAMEWORKBUNDLEID="${BUNDLEID}.framework.${DYLIB_LIBNAME}"
  echo "CFBundleIdentifier is ${FRAMEWORKBUNDLEID}"
  echo "convert ${DYLIB_BASENAME} to ${DYLIB_LIBNAME}.framework"

  DEST_FRAMEWORK="${TARGET_FRAMEWORKS}/${DYLIB_LIBNAME}.framework"
  mkdir -p "${DEST_FRAMEWORK}"
  mkdir -p "${DEST_FRAMEWORK}/Headers"
  mkdir -p "${DEST_FRAMEWORK}/Modules"

  # framework plists are binary
  plutil -convert binary1 "${SEEDFRAMEWORKPLIST}" -o "${DEST_FRAMEWORK}/Info.plist"
  # set real CFBundleName
  plutil -replace CFBundleName -string "${DYLIB_LIBNAME}" "${DEST_FRAMEWORK}/Info.plist"
  # set real CFBundleVersion
  plutil -replace CFBundleVersion -string "${BUNDLE_REVISION}" "${DEST_FRAMEWORK}/Info.plist"
  # set real CFBundleIdentifier
  plutil -replace CFBundleIdentifier -string "${FRAMEWORKBUNDLEID}" "${DEST_FRAMEWORK}/Info.plist"
  # set real CFBundleExecutable
  plutil -replace CFBundleExecutable -string "${DYLIB_LIBNAME}" "${DEST_FRAMEWORK}/Info.plist"
  # move it (not copy)
  mv -f "${DYLIB}" "${DEST_FRAMEWORK}/${DYLIB_LIBNAME}"

  # fixup loader id/paths
  LC_ID_DYLIB="@rpath/${DYLIB_LIBNAME}.framework/${DYLIB_LIBNAME}"
  LC_RPATH1="@executable_path/Frameworks/${DYLIB_LIBNAME}.framework"
  LC_RPATH2="@loader_path/Frameworks/${DYLIB_LIBNAME}.framework"
  install_name_tool -id "${LC_ID_DYLIB}" "${DEST_FRAMEWORK}/${DYLIB_LIBNAME}"
  install_name_tool -add_rpath "${LC_RPATH1}" "${DEST_FRAMEWORK}/${DYLIB_LIBNAME}"
  install_name_tool -add_rpath "${LC_RPATH2}" "${DEST_FRAMEWORK}/${DYLIB_LIBNAME}"

  if [ "$STRIP_INSTALLED_PRODUCT" == "YES" ]; then
    strip -x "${DEST_FRAMEWORK}/${DYLIB_LIBNAME}"
  fi

  if [ "$ACTION" == install ]; then
    # extract the uuid and use it to find the matching bcsymbolmap (needed for crashlog symbolizing)
    UUID=$(otool -l "${DEST_FRAMEWORK}/${DYLIB_LIBNAME}" | grep uuid | awk '{ print $2}')
    echo "bcsymbolmap is ${UUID}"
    if [ -f "${XBMC_DEPENDS}/bcsymbolmaps/${UUID}.bcsymbolmap" ]; then
      echo "bcsymbolmap is ${UUID}.bcsymbolmap"
      cp -f "${XBMC_DEPENDS}/bcsymbolmaps/${UUID}.bcsymbolmap" "${BUILT_PRODUCTS_DIR}/"
    fi
  fi
}

# todo: convert ios to soft frameworks as well to remove this if guard
if [ "$PLATFORM_NAME" == "appletvos" ]; then
  # loop over all xxx.dylibs in xxx.app/Frameworks
  for dylib in $(find "${TARGET_FRAMEWORKS}" -name "*.dylib" -type f); do
    convert2framework "${dylib}"
  done
fi
