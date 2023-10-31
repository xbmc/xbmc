#!/usr/bin/env bash

# credits:
# https://scriptingosx.com/2019/09/notarize-a-command-line-tool/
# https://developer.apple.com/documentation/technotes/tn3147-migrating-to-the-latest-notarization-tool

set -e

if [[ -z "$NOTARYTOOL_KEYCHAIN_PROFILE" ]]; then
  echo "skipping notarization"
  exit 0
fi

dmg="$1"
xcrun notarytool submit \
  --keychain-profile "$NOTARYTOOL_KEYCHAIN_PROFILE" \
  ${NOTARYTOOL_KEYCHAIN_PATH:+--keychain "$NOTARYTOOL_KEYCHAIN_PATH"} \
  --wait --timeout '1h' \
  "$dmg" 2>&1
xcrun stapler staple "$dmg"
