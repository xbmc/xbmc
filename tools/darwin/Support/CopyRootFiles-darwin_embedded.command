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
ADDONSYNC="rsync -aq --no-links --exclude .git* --exclude CVS* --exclude .svn* --exclude .cvsignore* --exclude .cvspass* --exclude .DS_Store* --exclude addons/skin.estuary --exclude addons/skin.estouchy --exclude addons/lib --exclude addons/share"

# binary name is Kodi but we build Kodi.bin so to get a clean binary each time
mv $TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/$EXECUTABLE_NAME.bin $TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/$EXECUTABLE_NAME

mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/media"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/system"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/userdata"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/media"
mkdir -p "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/tools/darwin/runtime"

${SYNC} "$BUILD_ROOT/LICENSE.md"  "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/"
${SYNC} "$BUILD_ROOT/privacy-policy.txt"  "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"
${SYNC} "$BUILD_ROOT/xbmc/platform/darwin/Credits.html"  "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/"
${ADDONSYNC} "$BUILD_ROOT/addons"  "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"
${SYNC} "$BUILD_ROOT/media"    "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"

# sync skin.estouchy
SYNCSKIN_A=${SKINSYNC}
if [ -f "$BUILD_ROOT/addons/skin.estouchy/media/Textures.xbt" ]; then
SYNCSKIN_A="${SKINSYNC} --exclude *.png --exclude *.jpg"
fi
${SYNCSKIN_A} "$BUILD_ROOT/addons/skin.estouchy"    "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons"
${SYNC} "$BUILD_ROOT/addons/skin.estouchy/background"   "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons/skin.estouchy"
${SYNC} "$BUILD_ROOT/addons/skin.estouchy/resources"   "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons/skin.estouchy"

# sync skin.estuary
SYNCSKIN_B=${SKINSYNC}
if [ -f "$BUILD_ROOT/addons/skin.estuary/media/Textures.xbt" ]; then
SYNCSKIN_B="${SKINSYNC} --exclude *.png --exclude *.jpg"
fi
${SYNCSKIN_B} "$BUILD_ROOT/addons/skin.estuary"     "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons"
${SYNC} "$BUILD_ROOT/addons/skin.estuary/extras"   "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons/skin.estuary"
${SYNC} "$BUILD_ROOT/addons/skin.estuary/resources"   "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome/addons/skin.estuary"

${SYNC} "$BUILD_ROOT/system"     "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"
${SYNC} "$BUILD_ROOT/userdata"   "$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH/AppData/AppHome"

fi
