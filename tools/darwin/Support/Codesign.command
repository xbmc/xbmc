#!/bin/bash

set -x

#this is the list of binaries we have to sign for being able to run un-jailbroken
LIST_BINARY_EXTENSIONS="dylib so"

export CODESIGN_ALLOCATE=`xcodebuild -find codesign_allocate`

GEN_ENTITLEMENTS="$NATIVEPREFIX/bin/gen_entitlements.py"
IOS11_ENTITLEMENTS="$XBMC_DEPENDS/share/ios11_entitlements.xml"
LDID32="$NATIVEPREFIX/bin/ldid32"
LDID64="$NATIVEPREFIX/bin/ldid64"
LDID=${LDID32}

if [ "${CURRENT_ARCH}" == "arm64" ] || [ "${CURRENT_ARCH}" == "aarch64" ]; then
  LDID=${LDID64}
  echo "using LDID64"
else
  echo "using LDID32"
fi

if [ ! -f ${GEN_ENTITLEMENTS} ]; then
  echo "error: $GEN_ENTITLEMENTS not found. Codesign won't work."
  exit -1
fi


if [ "${PLATFORM_NAME}" == "iphoneos" ] || [ "${PLATFORM_NAME}" == "appletvos" ]; then
  if [ -f "/Users/Shared/buildslave/keychain_unlock.sh" ]; then
    /Users/Shared/buildslave/keychain_unlock.sh
  fi

  #do fake sign - needed for jailbroken ios5.1 devices for some reason
  if [ -f ${LDID} ]; then
    find ${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/ -name "*.dylib" | xargs ${LDID} -S${IOS11_ENTITLEMENTS}
    find ${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/ -name "*.so" | xargs ${LDID} -S${IOS11_ENTITLEMENTS}
    ${LDID} -S${IOS11_ENTITLEMENTS} ${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/${APP_NAME}
    
    #repackage python eggs
    EGGS=`find ${CODESIGNING_FOLDER_PATH} -name "*.egg" -type f`
        for i in $EGGS; do
          echo $i
          mkdir del
          unzip -q $i -d del
          find ./del/ -name "*.so" -type f | xargs ${LDID} -S${IOS11_ENTITLEMENTS}
          rm $i
          cd del && zip -qr $i ./* &&  cd ..
          rm -r ./del/
        done
  fi

  # pull the CFBundleIdentifier out of the built xxx.app
  BUNDLEID=`mdls -raw -name kMDItemCFBundleIdentifier ${CODESIGNING_FOLDER_PATH}`
  if [ "${BUNDLEID}" == "(null)" ] ; then
    BUNDLEID=`/usr/libexec/PlistBuddy -c 'Print CFBundleIdentifier' ${CODESIGNING_FOLDER_PATH}/Info.plist`
  fi

  echo "CFBundleIdentifier is ${BUNDLEID}"


  # Prefer the expanded name, if available.
  CODE_SIGN_IDENTITY_FOR_ITEMS="${EXPANDED_CODE_SIGN_IDENTITY_NAME}"
 if [ "${CODE_SIGN_IDENTITY_FOR_ITEMS}" = "" ] ; then
   # Fall back to old behavior.
   CODE_SIGN_IDENTITY_FOR_ITEMS="${CODE_SIGN_IDENTITY}"
 fi
 echo "${CODE_SIGN_IDENTITY_FOR_ITEMS}"

  ${GEN_ENTITLEMENTS} "${BUNDLEID}" "${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/${PROJECT_NAME}.xcent";
  codesign -v -f -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" --entitlements "${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/${PROJECT_NAME}.xcent" "${BUILT_PRODUCTS_DIR}/${WRAPPER_NAME}/"

  #if user has set a code_sign_identity different from iPhone Developer we do a real codesign (for deployment on non-jailbroken devices)
  if ! [ -z "${CODE_SIGN_IDENTITY}" ] && [ "${CODE_SIGN_IDENTITY}" == "iPhone Developer" ] && [ "${CODE_SIGN_IDENTITY}" != "Don't Code Sign"  ]; then
    echo "Doing a full bundle sign using genuine identity ${CODE_SIGN_IDENTITY}"
    for binext in $LIST_BINARY_EXTENSIONS
    do
      codesign --deep -fvvv -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -i "${BUNDLEID}" `find ${CODESIGNING_FOLDER_PATH} -name "*.$binext" -type f` ${CODESIGNING_FOLDER_PATH}
    done
    echo "In case your app crashes with SIG_SIGN check the variable LIST_BINARY_EXTENSIONS in tools/darwin/Support/Codesign.command"

    #repackage python eggs
    EGGS=`find ${CODESIGNING_FOLDER_PATH} -name "*.egg" -type f`
    for i in $EGGS; do
      echo $i
      mkdir del
      unzip -q $i -d del
      codesign --deep -fvvv -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -i "${BUNDLEID}" `find ./del/ -name "*.$binext" -type f` ./del/
      rm $i
      cd del && zip -qr $i ./* &&  cd ..
      rm -r ./del/
    done
  fi
fi
