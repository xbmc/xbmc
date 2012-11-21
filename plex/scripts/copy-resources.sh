#!/bin/bash

echo "copy root files"

TARGET_NAME=$1
SRCROOT=$2
TARGET_BUILD_DIR=$3

echo "From $SRCROOT to $TARGET_BUILD_DIR/$TARGET_NAME"

# rsync command with exclusions for items we don't want in the app package
SYNC="rsync -aq --exclude .DS_Store* --exclude *.dll --exclude *.DLL --exclude *arm-osx.* --exclude *.zlib --exclude *.a"

# rsync command for excluding pngs and jpgs as well. Note that if the skin itself is not compiled
# using XBMCTex then excluding the pngs and jpgs will most likely make the skin unusable 
SYNCSKIN="rsync -aq --exclude CVS* --exclude .svn* --exclude .cvsignore* --exclude .cvspass* --exclude .DS_Store* --exclude *.dll  --exclude *.DLL --exclude *.png --exclude *.jpg --exclude *.bat --exclude Media* --exclude .git*"

# rsync command for including everything but the skins
ADDONSYNC="rsync -aq --exclude .DS_Store* --exclude skin.confluence --exclude skin.touched --exclude skin.mediastream"

mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/addons"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/language"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/media"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/sounds"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/system"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/userdata"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/media"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/extras/user"

${SYNC} "$SRCROOT/LICENSE.GPL" 	"$TARGET_BUILD_DIR/$TARGET_NAME/"
${SYNC} "$SRCROOT/xbmc/osx/Credits.html" 	"$TARGET_BUILD_DIR/$TARGET_NAME/"
${ADDONSYNC} "$SRCROOT/addons"		"$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
${ADDONSYNC} "$SRCROOT/plex/addons" "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
${SYNC} "$SRCROOT/language"		"$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
${SYNC} "$SRCROOT/media" 		"$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
${SYNC} "$SRCROOT/sounds" 		"$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
${SYNC} "$SRCROOT/system" 		"$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
${SYNC} "$SRCROOT/userdata" 	"$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
${SYNCSKIN} "$SRCROOT/addons/skin.mediastream" 	"$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/addons"
${SYNCSKIN} "$SRCROOT/addons/skin.plex" 	"$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/addons"

# copy extra packages if applicable
if [ -d "$SRCROOT/extras/system" ]; then
	${SYNC} "$SRCROOT/extras/system/" "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC"
fi

# copy extra user packages if applicable
if [ -d "$SRCROOT/extras/user" ]; then
	${SYNC} "$SRCROOT/extras/user/" "$TARGET_BUILD_DIR/$TARGET_NAME/XBMC/extras/user"
fi

# magic that gets the icon to update
touch "$TARGET_BUILD_DIR/$TARGET_NAME"

# not sure we want to do this with out major testing, many scripts cannot handle the spaces in the app name
#mv "$TARGET_BUILD_DIR/$TARGET_NAME" "$TARGET_BUILD_DIR/XBMC Media Center.app"

