#!/bin/bash

export CODESIGN_ALLOCATE=`xcodebuild -find codesign_allocate`

GEN_ENTITLEMENTS="/Developer/iphoneentitlements401/gen_entitlements.py"

if [ ! -f ${GEN_ENTITLEMENTS} ]; then
  echo "error: $GEN_ENTITLEMENTS not found. Codesign won't work."
  exit -1
fi


if [ "${PLATFORM_NAME}" == "iphoneos" ]; then
  if [ -f "/Users/Shared/buildslave/keychain_unlock.sh" ]; then
    /Users/Shared/buildslave/keychain_unlock.sh
  fi
  ${GEN_ENTITLEMENTS} "org.xbmc.xbmc-ios" "${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/${PROJECT_NAME}.xcent";
  codesign -v -f -s "iPhone Developer" --entitlements "${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/${PROJECT_NAME}.xcent" "${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/"
fi