#!/bin/bash

set -x

# This is the list of binaries we have to sign for being able to run un-jailbroken
LIST_BINARY_EXTENSIONS="dylib so app"

DARWIN_EMBEDDED_ENTITLEMENTS="$XBMC_DEPENDS/share/darwin_embedded_entitlements.xml"
LDID="$NATIVEPREFIX/bin/ldid"

if [ "${PLATFORM_NAME}" == "macosx" ]; then
  MACOS=1
fi

if [ "$MACOS" ]; then
  CONTENTS_PATH="${CODESIGNING_FOLDER_PATH}/Contents"
else
  CONTENTS_PATH="${CODESIGNING_FOLDER_PATH}"
fi

# Pull the CFBundleIdentifier out of the built xxx.app
BUNDLEID=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "${CONTENTS_PATH}/Info.plist")
echo "CFBundleIdentifier is '${BUNDLEID}'"

# Prefer the expanded name, if available.
CODE_SIGN_IDENTITY_FOR_ITEMS="${EXPANDED_CODE_SIGN_IDENTITY_NAME}"
if [ "${CODE_SIGN_IDENTITY_FOR_ITEMS}" = "" ] ; then
  # Fall back to old behavior.
  CODE_SIGN_IDENTITY_FOR_ITEMS="${CODE_SIGN_IDENTITY}"
fi
echo "Code sign identity is '${CODE_SIGN_IDENTITY_FOR_ITEMS}'"

# Delete existing codesign and provisioning file
rm -f "${CONTENTS_PATH}/embedded.mobileprovision"
rm -rf "${CONTENTS_PATH}/_CodeSignature"

# If user has set a code_sign_identity we do a real codesign (for deployment on non-jailbroken devices)
if ! [ -z "${CODE_SIGN_IDENTITY_FOR_ITEMS}" ]; then
  if egrep -q --max-count=1 -e '^iPhone (Developer|Distribution): ' -e '^Apple (Development|Distribution): ' -e '^[[:xdigit:]]+$' -e '^Developer ID Application: ' <<<"${CODE_SIGN_IDENTITY_FOR_ITEMS}"; then
    echo "Doing a full bundle sign using genuine identity '${CODE_SIGN_IDENTITY_FOR_ITEMS}'"

    for binext in $LIST_BINARY_EXTENSIONS
    do
      echo "Signing binaries with '$binext' extension"
      # Check if at least 1 file with the extension exists to sign, otherwise do nothing
      FINDOUTPUT=$(find "${CONTENTS_PATH}" -iname "*.$binext" -type f)
      if [ `echo $FINDOUTPUT | wc -l` != 0 ]; then
        for singlefile in $FINDOUTPUT; do
          codesign -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -fvvv -i "${BUNDLEID}" "${singlefile}"
        done
      fi
    done

    for FRAMEWORK_PATH in $(find "${CONTENTS_PATH}" -iname "*.framework" -type d)
    do
      DYLIB_BASENAME=$(basename "${FRAMEWORK_PATH%.framework}")
      echo "Signing Framework: ${DYLIB_BASENAME}.framework"
      FRAMEWORKBUNDLEID="${BUNDLEID}.framework.${DYLIB_BASENAME}"
      codesign -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -fvvv -i "${FRAMEWORKBUNDLEID}" "${FRAMEWORK_PATH}/${DYLIB_BASENAME}"
      codesign -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -fvvv -i "${FRAMEWORKBUNDLEID}" "${FRAMEWORK_PATH}"
    done

    if [ "$MACOS" ]; then
      # Sign and repackage python eggs for osx
      EGGS=$(find "${CONTENTS_PATH}" -iname "*.egg" -type f)
      echo "Signing Eggs"
      for i in $EGGS; do
        echo $i
        mkdir del
        unzip -q $i -d del
        for binext in $LIST_BINARY_EXTENSIONS
        do
          # Check if at least 1 file with the extension exists to sign, otherwise do nothing
          FINDOUTPUT=$(find ./del/ -iname "*.$binext" -type f)
          if [ `echo $FINDOUTPUT | wc -l` != 0 ]; then
            for singlefile in $FINDOUTPUT; do
              codesign -s "${CODE_SIGN_IDENTITY_FOR_ITEMS}" -fvvv -i "${BUNDLEID}" "${singlefile}"
            done
          fi
        done
        rm $i
        cd del && zip -qr $i ./* && cd ..
        rm -r ./del/
      done
    fi
  fi
elif [ ! "$MACOS" ]; then
  # Do fake sign - needed for iOS >=5.1 and tvOS >=10.2 jailbroken devices
  # See http://www.saurik.com/id/8
  echo "Doing a fake sign using ldid for jailbroken devices (main kodi binary and all Mach-O files)"

  # Main 'kodi' binary
  "${LDID}" -S"${DARWIN_EMBEDDED_ENTITLEMENTS}" "${CONTENTS_PATH}/${EXECUTABLE_NAME}"

  # All Mach-O files (except TopShelf)
  for f in $(find "${CONTENTS_PATH}/AppData" "${CONTENTS_PATH}/Frameworks" -type f); do
    if [[ $(file ${f}) == *"Mach-O"* ]]; then
      "${LDID}" -S "${f}"
    fi
  done
fi
