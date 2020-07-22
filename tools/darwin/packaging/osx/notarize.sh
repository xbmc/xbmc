#!/usr/bin/env bash

# credits: https://scriptingosx.com/2019/09/notarize-a-command-line-tool/

if [[ -z "$DEV_ACCOUNT" || -z "$DEV_ACCOUNT_PASSWORD" ]]; then
  echo "skipping notarization"
  exit 0
fi

notarizefile() { # $1: path to file to notarize, $2: identifier
  filepath=${1:?"need a filepath"}
  identifier=${2:?"need an identifier"}

  # upload file
  echo "uploading $filepath for notarization"
  altoolOutput=$(xcrun altool \
    --notarize-app \
    --type osx \
    --file "$filepath" \
    --primary-bundle-id "$identifier" \
    --username "$DEV_ACCOUNT" \
    --password "$DEV_ACCOUNT_PASSWORD" \
    ${DEV_TEAM:+--asc-provider "$DEV_TEAM"} 2>&1)

  requestUUID=$(echo "$altoolOutput" | awk '/RequestUUID/ { print $NF; }')

  if [[ $requestUUID == "" ]]; then
    echo "Failed to upload:"
    echo "$altoolOutput"
    return 1
  fi
  echo "requestUUID: $requestUUID, waiting..."

  # wait for status to be not "in progress" any more
  request_status="in progress"
  while [[ "$request_status" == "in progress" ]]; do
    sleep 60
    altoolOutput=$(xcrun altool \
      --notarization-info "$requestUUID" \
      --username "$DEV_ACCOUNT" \
      --password "$DEV_ACCOUNT_PASSWORD" 2>&1)
    request_status=$(echo "$altoolOutput" | awk -F ': ' '/Status:/ { print $2; }' )
  done

  # print status information
  echo "$altoolOutput"

  if [[ $request_status != "success" ]]; then
    echo "warning: could not notarize $filepath"
    notarizationFailed=1
  fi

  LogFileURL=$(echo "$altoolOutput" | awk -F ': ' '/LogFileURL:/ { print $2; }')
  if [[ "$LogFileURL" ]]; then
    echo -e "\nnotarization details:"
    curl "$LogFileURL"
    echo
  fi
  if [[ $notarizationFailed == 1 ]]; then
    return 1
  fi
  return 0
}

dmg="$1"
notarizefile "$dmg" $(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$2") \
  && xcrun stapler staple "$dmg"
