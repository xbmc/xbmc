#!/usr/bin/env bash

#  Copyright (C) 2022 Team Kodi
#  This file is part of Kodi - https://kodi.tv
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#  See LICENSES/README.md for more information.

appPath="$1"
appName="$2"
dmgPath="$3"

if [[ ! -d "$appPath" ]]; then
  echo "app not found at $appPath"
  exit 1
fi

tempDir=$(mktemp -d -t "$appName")

# prepare DMG contents
ditto "$appPath" "$tempDir/$appName.app"
ditto VolumeIcon.icns "$tempDir/.VolumeIcon.icns"
cp VolumeDSStoreApp "$tempDir/.DS_Store"
ln -s /Applications "$tempDir/Applications"

destBackgroundPath="$tempDir/background"
ditto ../media/osx/background "$destBackgroundPath"
xcrun SetFile -a V "$destBackgroundPath"

# volume icon attribute must be set on the mounted DMG
dmgFileTemp="$(mktemp -t "$appName").dmg"
hdiutil create -fs HFS+ -format UDRW -volname "$appName" -srcfolder "$tempDir" "$dmgFileTemp"
# deviceHandle is /dev/disk<number>
deviceHandle=$(hdiutil attach -readwrite -noverify -noautoopen "$dmgFileTemp" \
  | fgrep GUID_partition_scheme | awk -F '[[:space:]]' '{print $1}')
xcrun SetFile -a C "/Volumes/$appName"

# compress DMG
diskutil eject "$deviceHandle"
hdiutil convert "$dmgFileTemp" -format ULFO -o "$dmgPath"

rm -rf "$dmgFileTemp" "$tempDir"
