#!/bin/bash

set -x

LDID="$NATIVEPREFIX/bin/ldid"

# Delete existing codesign and provisioning file
rm -f "${CODESIGNING_FOLDER_PATH}/embedded.mobileprovision"
rm -rf "${CODESIGNING_FOLDER_PATH}/_CodeSignature"

# If user has not set a code_sign_identity we do a fake sign
if [ -z "${CODE_SIGN_IDENTITY}" ]; then
  # Do fake sign - needed for iOS >=5.1 and tvOS >=10.2 jailbroken devices
  # See http://www.saurik.com/id/8
  echo "Doing a fake sign of Top Shelf binary using ldid for jailbroken devices"
  "${LDID}" -S "${CODESIGNING_FOLDER_PATH}/${EXECUTABLE_NAME}"
fi
