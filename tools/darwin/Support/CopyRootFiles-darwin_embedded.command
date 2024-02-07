#!/bin/bash

set -eux

echo "copy root files"

if [ "$ACTION" = build ] ; then

# rsync command with exclusions for items we don't want in the app package
SYNC="rsync -aq --exclude .git* --exclude .DS_Store* --exclude *.dll --exclude *.DLL --exclude *linux.* --exclude *x86-osx.so --exclude *.zlib --exclude *.a"

# rsync command for skins. jpg, png exclusion is handled during sync
# if a Textures.xbt file is found
SKINSYNC="rsync -aq --exclude .git* --exclude CVS* --exclude .svn* --exclude .cvsignore* --exclude .cvspass* --exclude .DS_Store* --exclude *.dll  --exclude *.DLL --exclude *linux.*  --exclude *.bat"

# rsync command for including everything but the skins
ADDONSYNC="rsync -aq --no-links --exclude .git* --exclude CVS* --exclude .svn* --exclude .cvsignore* --exclude .cvspass* --exclude .DS_Store* --exclude addons/skin.estuary --exclude addons/lib --exclude addons/share"

# binary name is Kodi but we build Kodi.bin so to get a clean binary each time
mv $TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/$EXECUTABLE_NAME.bin $TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/$EXECUTABLE_NAME

mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/media"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/system"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/userdata"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/media"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/tools/darwin/runtime"

${SYNC} "$SRCROOT/LICENSE.md"  "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/"
${SYNC} "$SRCROOT/privacy-policy.txt"  "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"
${SYNC} "$SRCROOT/xbmc/platform/darwin/Credits.html"  "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/"
${ADDONSYNC} "$SRCROOT/addons"  "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"
${SYNC} "$SRCROOT/media"    "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"

# extracted eggs
${SYNC} "$XBMC_DEPENDS/share/$APP_NAME/addons" "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"

# sync skin.estuary
SYNCSKIN_B=${SKINSYNC}
if [ -f "$SRCROOT/addons/skin.estuary/media/Textures.xbt" ]; then
SYNCSKIN_B="${SKINSYNC} --exclude *.png --exclude *.jpg"
fi
${SYNCSKIN_B} "$SRCROOT/addons/skin.estuary"     "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons"
${SYNC} "$SRCROOT/addons/skin.estuary/extras"   "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons/skin.estuary"
${SYNC} "$SRCROOT/addons/skin.estuary/resources"   "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons/skin.estuary"

${SYNC} "$SRCROOT/system"     "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"
${SYNC} "$SRCROOT/userdata"   "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"

fi
