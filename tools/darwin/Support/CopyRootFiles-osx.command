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
ADDONSYNC="rsync -aq --no-links --exclude .git* --exclude .DS_Store* --exclude addons/skin.estuary --exclude addons/skin.estouchy"

mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/addons"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/media"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/system"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/userdata"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/media"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/tools/darwin/runtime"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/extras/user"

${SYNC} "$SRCROOT/LICENSE.GPL" 	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/"
${SYNC} "$SRCROOT/xbmc/platform/darwin/Credits.html" 	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/"
${SYNC} "$SRCROOT/tools/darwin/runtime"	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/tools/darwin"
${ADDONSYNC} "$SRCROOT/addons"		"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME"
${SYNC} "$SRCROOT/media" 		"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME"
${SYNCSKIN} "$SRCROOT/addons/skin.estuary" 	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/addons"
${SYNC} "$SRCROOT/addons/skin.estuary/extras" 	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/addons/skin.estuary"
${SYNC} "$SRCROOT/addons/skin.estuary/resources"   "$TARGET_BUILD_DIR/$TARGET_NAME/AppData/AppHome/addons/skin.estuary"
${SYNCSKIN} "$SRCROOT/addons/skin.estouchy" 	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/addons"
${SYNC} "$SRCROOT/addons/skin.estouchy/background" 	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/addons/skin.estouchy"
${SYNC} "$SRCROOT/addons/skin.estouchy/resources" 	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/addons/skin.estouchy"
${SYNC} "$SRCROOT/system" 		"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME"
${SYNC} "$SRCROOT/userdata" 	"$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME"

# copy extra packages if applicable
if [ -d "$SRCROOT/extras/system" ]; then
	${SYNC} "$SRCROOT/extras/system/" "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME"
fi

# copy extra user packages if applicable
if [ -d "$SRCROOT/extras/user" ]; then
	${SYNC} "$SRCROOT/extras/user/" "$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/$APP_NAME/extras/user"
fi



# magic that gets the icon to update
touch "$TARGET_BUILD_DIR/$TARGET_NAME"

# not sure we want to do this with out major testing, many scripts cannot handle the spaces in the app name
#mv "$TARGET_BUILD_DIR/$TARGET_NAME" "$TARGET_BUILD_DIR/$APP_NAME Media Center.app"

fi
