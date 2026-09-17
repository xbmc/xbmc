#!/bin/bash

set -ux

EXTERNAL_LIBS="$XBMC_DEPENDS"

TARGET_BINARY="$TARGET_BUILD_DIR/$EXECUTABLE_PATH"
TARGET_CONTENTS="$TARGET_BUILD_DIR/$FULL_PRODUCT_NAME"
TARGET_FRAMEWORKS="$TARGET_BUILD_DIR/$FRAMEWORKS_FOLDER_PATH"

DYLIB_NAMEPATH="@executable_path/Frameworks"
XBMC_HOME="$TARGET_CONTENTS/AppData/AppHome"

function log
{
  set +x
  echo "****************************************"
  echo "$@"
  echo "****************************************"
  set -x
}

function build_framework_name
{
  # stem
  basename "${1%.*}"
}

function build_framework_name_python
{
  # <site-packages>/Cryptodome/PublicKey/_curve25519.abi3.so -> Cryptodome.PublicKey._curve25519.abi3
  relativeToSitePackages="${1#"$pythonSitePackagesDir/"}"
  withoutExtension="${relativeToSitePackages%.*}"
  echo "${withoutExtension//\//.}"
}

# "returns" variable FRAMEWORK_BINARY_PATH
function check_xbmc_dylib_depends
{
  local binaryPath="$1"
  buildFrameworkNameFunc="$2"

  # process dependencies first, check only those that are from external depends
  # NR>3 skips first 3 static lines, example:
  # <binary path> [arm64]:
  #  -linked_dylibs:
  #      attributes     load path
  local changeRpathCommands=''
  while IFS= read -r externalLibPath; do
    frameworkName=$($buildFrameworkNameFunc "$externalLibPath")
    changeRpathCommands+="-change $externalLibPath $DYLIB_NAMEPATH/$frameworkName.framework/$frameworkName "
    # TODO: external libs must be copied to app bundle first
    # TODO: when processing a python native module, external libs must use static `build_framework_name`
    check_xbmc_dylib_depends "$externalLibPath" "$buildFrameworkNameFunc"
  done < <(dyld_info -linked_dylibs "$binaryPath" | awk -v p="$EXTERNAL_LIBS" 'NR>3 && index($1, p) { print $1 }')
  [ -n "$changeRpathCommands" ] && install_name_tool $changeRpathCommands "$binaryPath"

  # put binary to Frameworks
  if [ -z "${3:-}" ] ; then
    frameworkName=$($buildFrameworkNameFunc "$binaryPath")
    framework="$frameworkName.framework"
    frameworkBinaryPath="$framework/$frameworkName"

    FRAMEWORK_BINARY_PATH="$frameworkBinaryPath"

    log "'$binaryPath' -> '$frameworkBinaryPath'"
    if [ ! -d "$framework" ]; then
      log "framework '$framework' doesn't exist yet, creating it"
      mkdir "$framework"
      if [[ $binaryPath == "$EXTERNAL_LIBS"* ]]; then
        cp "$binaryPath" "$frameworkBinaryPath"
      else
        mv "$binaryPath" "$frameworkBinaryPath"
      fi
      install_name_tool -id "$DYLIB_NAMEPATH/$frameworkBinaryPath" "$frameworkBinaryPath"

      # bundle ID must contain only dots, hyphens and alphanumerics
      set +x
      echo "$frameworkInfoPlistBase
	<key>CFBundleExecutable</key>
	<string>$frameworkName</string>
	<key>CFBundleIdentifier</key>
	<string>$PRODUCT_BUNDLE_IDENTIFIER.${frameworkName//_/-}</string>
</dict>
</plist>" > "$framework/Info.plist"
      set -x
    fi
  else
    FRAMEWORK_BINARY_PATH=
  fi
}

function check_xbmc_dylib_depends_in_dir
{
  dir="$1"
  buildFrameworkNameFunc="$2"
  extraProcessorFunc="${3:-}"
  while IFS= read -r -d '' libPath ; do
    check_xbmc_dylib_depends "$libPath" "$buildFrameworkNameFunc"
    [ -z "$extraProcessorFunc" ] || "$extraProcessorFunc" "$libPath"
  done < <(find "$dir" -type f \( -iname '*.dylib' -or -iname '*.so' \) -print0)
}

function package_python_lib
{
  log "Creating required files for a python framework lib"
  libPath="$1"
  libPathFwork="${libPath%.*}.fwork"
  echo "${libPathFwork#"$TARGET_CONTENTS/"}" > "$FRAMEWORK_BINARY_PATH.origin"
  echo "Frameworks/$FRAMEWORK_BINARY_PATH" > "$libPathFwork"
}


# main script

frameworkInfoPlistBase='<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDevelopmentRegion</key>
	<string>en</string>
	<key>CFBundleInfoDictionaryVersion</key>
	<string>6.0</string>
	<key>CFBundlePackageType</key>
	<string>FMWK</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0</string>
	<key>CFBundleSignature</key>
	<string>????</string>
	<key>CFBundleVersion</key>
	<string>1</string>'

# PLATFORM_DIR=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform
frameworkInfoPlistBase+="
	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>$(basename "$PLATFORM_DIR" .platform)</string>
	</array>"

# TARGETED_DEVICE_FAMILY=1,2
frameworkInfoPlistBase+="
	<key>UIDeviceFamily</key>
	<array>"
IFS=, read -ra deviceFamilies <<< "$TARGETED_DEVICE_FAMILY"
for deviceFamily in "${deviceFamilies[@]}" ; do
  frameworkInfoPlistBase+="
		<integer>$deviceFamily</integer>"
done
frameworkInfoPlistBase+="
	</array>"

# just copy entries from the app's Info.plist
for key in BuildMachineOSBuild DTCompiler DTPlatformBuild DTPlatformName DTPlatformVersion DTSDKBuild DTSDKName DTXcode DTXcodeBuild MinimumOSVersion ; do
  frameworkInfoPlistBase+="
	<key>$key</key>
	<string>$(/usr/libexec/PlistBuddy "$TARGET_BUILD_DIR/$INFOPLIST_PATH" -c "Print :$key")</string>"
done


mkdir -p "$TARGET_CONTENTS"
mkdir -p "$TARGET_CONTENTS/AppData/AppHome"
# start clean so we don't keep old dylibs
rm -rf "$TARGET_FRAMEWORKS"
mkdir -p "$TARGET_FRAMEWORKS"

pythonDir="python$PYTHON_VERSION"
pythonSrc="$EXTERNAL_LIBS/lib/$pythonDir"
pythonDst="$TARGET_CONTENTS/lib/$pythonDir"

log "Package $pythonSrc"
rm -rf "$pythonDst"
PYTHONSYNC="rsync -aq --exclude .DS_Store --exclude *.a --exclude *.exe --exclude test --exclude tests"
${PYTHONSYNC} "$pythonSrc/" "$pythonDst/"
rm -rf "$pythonDst/config"

cd "$TARGET_FRAMEWORKS"

log "Checking $FULL_PRODUCT_NAME for dylib dependencies"
check_xbmc_dylib_depends "$TARGET_BINARY" build_framework_name 1

for dir in addons system ; do
  log "Checking '$dir' for dylib dependencies"
  check_xbmc_dylib_depends_in_dir "$XBMC_HOME/$dir" build_framework_name
done

# TODO: enable for tvOS once Python is built as a real tvOS platform
if [[ $PLATFORM_NAME == iphone* ]] ; then
  log "Packaging python packages as frameworks"
  pythonSitePackagesDir="$pythonDst/site-packages"
  check_xbmc_dylib_depends_in_dir "$pythonSitePackagesDir" build_framework_name_python package_python_lib
fi
