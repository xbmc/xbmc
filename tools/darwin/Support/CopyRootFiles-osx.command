#!/bin/bash

echo "copy root files"

if [ "$ACTION" = build ] ; then

# for external testing
#TARGET_NAME=$APP_NAME.app
#SRCROOT=/Users/Shared/xbmc_svn/$APP_NAME
#TARGET_BUILD_DIR=/Users/Shared/xbmc_svn/$APP_NAME/build/Debug

# rsync command with exclusions for items we don't want in the app package
SYNC="rsync -aq --exclude .git* --exclude .DS_Store* --exclude *.dll --exclude *.DLL --exclude *linux.* --exclude *arm-osx.* --exclude *.zlib --exclude *.a"

# rsync command for excluding pngs and jpgs as well. Note that if the skin itself is not compiled
# using XBMCTex then excluding the pngs and jpgs will most likely make the skin unusable
SYNCSKIN="rsync -aq --exclude .git* --exclude CVS* --exclude .svn* --exclude .cvsignore* --exclude .cvspass* --exclude .DS_Store* --exclude *.dll  --exclude *.DLL --exclude *linux.* --exclude *.png --exclude *.jpg --exclude *.bat"

# rsync command for including everything but the skins
ADDONSYNC="rsync -aq --no-links --exclude .git* --exclude .DS_Store* --exclude addons/skin.estuary"

BASE_TARGET_PATH="$TARGET_BUILD_DIR/Resources"
TARGET_PATH="$BASE_TARGET_PATH/$APP_NAME"

mkdir -p "$TARGET_PATH"
mkdir -p "$TARGET_PATH/addons"
mkdir -p "$TARGET_PATH/media"
mkdir -p "$TARGET_PATH/system"
mkdir -p "$TARGET_PATH/userdata"
mkdir -p "$TARGET_PATH/media"
mkdir -p "$TARGET_PATH/tools/darwin/runtime"
mkdir -p "$TARGET_PATH/extras/user"

${SYNC} "$SRCROOT/LICENSE.md" "$BASE_TARGET_PATH"
${SYNC} "$SRCROOT/privacy-policy.txt" "$TARGET_PATH"
${SYNC} "$SRCROOT/xbmc/platform/darwin/Credits.html" "$BASE_TARGET_PATH"
${SYNC} "$SRCROOT/tools/darwin/runtime" "$TARGET_PATH/tools/darwin"
${ADDONSYNC} "$SRCROOT/addons" "$TARGET_PATH"
${SYNC} "$SRCROOT/media" "$TARGET_PATH"
${SYNCSKIN} "$SRCROOT/addons/skin.estuary" "$TARGET_PATH/addons"
${SYNC} "$SRCROOT/addons/skin.estuary/extras" "$TARGET_PATH/addons/skin.estuary"
${SYNC} "$SRCROOT/addons/skin.estuary/resources" "$TARGET_PATH/addons/skin.estuary"
${SYNC} --include 'settings/settings.xml' --include 'settings/darwin*' --exclude 'settings/*.xml' "$SRCROOT/system" "$TARGET_PATH"
${SYNC} "$SRCROOT/userdata" "$TARGET_PATH"

# copy extra packages if applicable
if [ -d "$SRCROOT/extras/system" ]; then
  ${SYNC} "$SRCROOT/extras/system/" "$TARGET_PATH"
fi

# copy extra user packages if applicable
if [ -d "$SRCROOT/extras/user" ]; then
  ${SYNC} "$SRCROOT/extras/user/" "$TARGET_PATH/extras/user"
fi

# not sure we want to do this with out major testing, many scripts cannot handle the spaces in the app name
#mv "$TARGET_BUILD_DIR/$TARGET_NAME" "$TARGET_BUILD_DIR/$APP_NAME Media Center.app"

fi
