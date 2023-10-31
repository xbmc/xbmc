#!/usr/bin/env bash

# credits:
# https://scriptingosx.com/2019/09/notarize-a-command-line-tool/
# https://developer.apple.com/documentation/technotes/tn3147-migrating-to-the-latest-notarization-tool

set -e

if [[ -z "$DEV_ACCOUNT" || -z "$DEV_ACCOUNT_PASSWORD" ]]; then
  echo "skipping notarization"
  exit 0
fi

dmg="$1"
xcrun notarytool \
  submit \
  --wait \
  --timeout '1h' \
  --apple-id "$DEV_ACCOUNT" \
  --password "$DEV_ACCOUNT_PASSWORD" \
  ${DEV_TEAM:+--team-id "$DEV_TEAM"} \
  "$dmg" 2>&1
xcrun stapler staple "$dmg"
