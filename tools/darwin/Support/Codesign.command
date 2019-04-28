#!/bin/bash

set -x

#this is the list of binaries we have to sign for being able to run un-jailbroken
LIST_BINARY_EXTENSIONS="dylib so 0 vis pvr app"

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

  # todo: is this required anymore?
  if [ "${PLATFORM_NAME}" == "iphoneos" ]; then
    #do fake sign - needed for jailbroken ios5.1 devices for some reason
    if [ -f ${LDID} ]; then
      find ${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/ -name "*.dylib" | xargs ${LDID} -S${IOS11_ENTITLEMENTS}
      find ${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/ -name "*.so" | xargs ${LDID} -S${IOS11_ENTITLEMENTS}
      ${LDID} -S${IOS11_ENTITLEMENTS} ${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/${EXECUTABLE_NAME}
    
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

  ${GEN_ENTITLEMENTS} "${BUNDLEID}" "${BUILT_PRODUCTS_DIR}/${EXECUTABLE_FOLDER_PATH}/${EXECUTABLE_NAME}.xcent";

  # delete existing codesigning
  if [ -d "${CODESIGNING_FOLDER_PATH}/_CodeSignature" ]; then
    rm -r ${CODESIGNING_FOLDER_PATH}/_CodeSignature
  fi
  if [ -f "${CODESIGNING_FOLDER_PATH}/embedded.mobileprovision" ]; then
    rm -f ${CODESIGNING_FOLDER_PATH}/embedded.mobileprovision
  fi

  #if user has set a code_sign_identity different from iPhone Developer we do a real codesign (for deployment on non-jailbroken devices)
  if ! [ -z "${CODE_SIGN_IDENTITY}" ] && echo ${CODE_SIGN_IDENTITY} | grep -cim1 "iPhone Developer" &>/dev/null; then
    echo "Doing a full bundle sign using genuine identity ${CODE_SIGN_IDENTITY}"
    for binext in $LIST_BINARY_EXTENSIONS
    do
      echo "Signing binary: $binext"
      # check if at least 1 file with the extension exists to sign, otherwise do nothing
      FINDOUTPUT=`find ${CODESIGNING_FOLDER_PATH} -name "*.$binext" -type f`
      if [ `echo $FINDOUTPUT | wc -l` != 0 ]; then
        for singlefile in $FINDOUTPUT; do
          codesign -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -fvvv -i "${BUNDLEID}" "${singlefile}"
        done
      fi
    done
    echo "In case your app crashes with SIG_SIGN check the variable LIST_BINARY_EXTENSIONS in tools/darwin/Support/Codesign.command"

    for FRAMEWORK_PATH in `find ${CODESIGNING_FOLDER_PATH} -name "*.framework" -type d`
    do
      DYLIB_BASENAME=$(basename "${FRAMEWORK_PATH%.framework}")
      echo "Signing Framework: ${DYLIB_BASENAME}.framework"
      FRAMEWORKBUNDLEID="${BUNDLEID}.framework.${DYLIB_BASENAME}"
      codesign -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -fvvv -i "${FRAMEWORKBUNDLEID}" ${FRAMEWORK_PATH}/${DYLIB_BASENAME}
      codesign -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -fvvv -i "${FRAMEWORKBUNDLEID}" ${FRAMEWORK_PATH}
    done

    #repackage python eggs
    EGGS=`find ${CODESIGNING_FOLDER_PATH} -name "*.egg" -type f`
    echo "Signing Eggs"
    for i in $EGGS; do
      echo $i
      mkdir del
      unzip -q $i -d del
      for binext in $LIST_BINARY_EXTENSIONS
      do
        # check if at least 1 file with the extension exists to sign, otherwise do nothing
        FINDOUTPUT=`find ./del/ -name "*.$binext" -type f`
        if [ `echo $FINDOUTPUT | wc -l` != 0 ]; then
          for singlefile in $FINDOUTPUT; do
            codesign -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -fvvv -i "${BUNDLEID}" "${singlefile}"
          done
        fi
      done
      rm $i
      cd del && zip -qr $i ./* &&  cd ..
      rm -r ./del/
    done
  fi
fi
