#!/bin/bash

echo "copy root files"

if [ "$ACTION" = build ] ; then

# for external testing
TARGET_NAME=$PRODUCT_NAME.$WRAPPER_EXTENSION
#SRCROOT=/Users/Shared/xbmc_svn/XBMC
#TARGET_BUILD_DIR=/Users/Shared/xbmc_svn/XBMC/build/Debug

# rsync command with exclusions for items we don't want in the app package
SYNC="rsync -aq --exclude .git* --exclude .DS_Store* --exclude *.dll --exclude *.DLL --exclude *linux.* --exclude *x86-osx.so --exclude *.zlib --exclude *.a"

# rsync command for skins. jpg, png exclusion is handled during sync
# if a Textures.xbt file is found
SKINSYNC="rsync -aq --exclude .git* --exclude CVS* --exclude .svn* --exclude .cvsignore* --exclude .cvspass* --exclude .DS_Store* --exclude *.dll  --exclude *.DLL --exclude *linux.*  --exclude *.bat"

# rsync command for including everything but the skins
ADDONSYNC="rsync -aq --exclude .git* --exclude CVS* --exclude .svn* --exclude .cvsignore* --exclude .cvspass* --exclude .DS_Store* --exclude addons/skin.confluence --exclude addons/skin.re-touched --exclude screensaver.rsxs* --exclude visualization.*"

# binary name is XBMC but we build XBMC.bin so to get a clean binary each time
mv $TARGET_BUILD_DIR/$TARGET_NAME/XBMC.bin $TARGET_BUILD_DIR/$TARGET_NAME/XBMC

mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/language"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/media"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/sounds"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/system"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/userdata"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/media"
mkdir -p "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/tools/darwin/runtime"

${SYNC} "$SRCROOT/LICENSE.GPL"  "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/"
${SYNC} "$SRCROOT/xbmc/osx/Credits.html"  "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/"
${ADDONSYNC} "$SRCROOT/addons"  "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome"
${SYNC} "$SRCROOT/addons/visualization.glspectrum"    "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons"
${SYNC} "$SRCROOT/addons/visualization.waveform"      "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons"
${SYNC} "$SRCROOT/language"   "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome"
${SYNC} "$SRCROOT/media"    "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome"

# sync touch skin if it exists
if [ -f "$SRCROOT/addons/skin.re-touched/addon.xml" ]; then
SYNCSKIN_A=${SKINSYNC}
if [ -f "$SRCROOT/addons/skin.re-touched/media/Textures.xbt" ]; then
SYNCSKIN_A="${SKINSYNC} --exclude *.png --exclude *.jpg"
fi
${SYNCSKIN_A} "$SRCROOT/addons/skin.re-touched"    "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons"
${SYNC} "$SRCROOT/addons/skin.re-touched/background"   "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons/skin.re-touched"
${SYNC} "$SRCROOT/addons/skin.re-touched/icon.png"   "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons/skin.re-touched"
fi

# sync skin.confluence
SYNCSKIN_B=${SKINSYNC}
if [ -f "$SRCROOT/addons/skin.confluence/media/Textures.xbt" ]; then
SYNCSKIN_B="${SKINSYNC} --exclude *.png --exclude *.jpg"
fi
${SYNCSKIN_B} "$SRCROOT/addons/skin.confluence"     "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons"
${SYNC} "$SRCROOT/addons/skin.confluence/backgrounds"   "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons/skin.confluence"
${SYNC} "$SRCROOT/addons/skin.confluence/icon.png"    "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome/addons/skin.confluence"

${SYNC} "$SRCROOT/sounds"     "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome"
${SYNC} "$SRCROOT/system"     "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome"
${SYNC} "$SRCROOT/userdata"   "$TARGET_BUILD_DIR/$TARGET_NAME/XBMCData/XBMCHome"

fi
