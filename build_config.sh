#!/usr/bin/bash

set -x
echo $SHELL
repo=v20icu
KODI_SOURCE=/scratch/frank/Source/"${repo}"
BUILD_DIR=/scratch/frank/Source/"${repo}".build/kodi-build-master
INSTALL_DIR=/scratch/frank/Source/"${repo}".build/master

CXX_COMPILER="/usr/bin/clang++-14"

CMAKE_XX_ARGS=()
CMAKE_XX_ARGS+=(-DCMAKE_CXX_STANDARD="17")
CMAKE_XX_ARGS+=(-DCMAKE_CXX_COMPILER="${CXX_COMPILER}")
CMAKE_XX_ARGS+=(-DCMAKE_CXX_COMPILER_ID="Clang")
CMAKE_XX_ARGS+=(-DCMAKE_CXX_COMPILER_VERSION="14")

# Work around CMake scripts not finding clang version info
CMAKE_XX_ARGS+=(-DCMAKE_CXX_STANDARD_COMPUTED_DEFAULT="17")

C_COMPILER_OPTIONS="-O0 -g -fno-optimize-sibling-calls -fno-inline-functions" 

declare -a CMAKE_CXX_FLAGS
CMAKE_CXX_FLAGS+=("${C_COMPILER_OPTIONS}")

C_COMPILER="/usr/bin/clang-14"

CMAKE_ARGS=()
CMAKE_ARGS+=(-DCMAKE_C_COMPILER="${C_COMPILER}")

declare -a CMAKE_C_FLAGS
CMAKE_C_FLAGS+=("${C_COMPILER_OPTIONS}")

ECM_ENABLE_SANITIZERS=""

# ICU config options
# Config and build of ICU done in build_icu.sh, but I put config
# options here to be consistent.
#
# You might find https://unicode-org.github.io/icu/userguide/icu4c/build.html
# useful. Also: https://icu.unicode.org/repository

#ICU_CONFIG_PLATFORM=Linux  # clang/clang++ or Gnu gcc/g++
ICU_CONFIG_PLATFORM=Linux/gcc  #  Gnu gcc/g++

declare -a ICU_CONFIG_ENVS

#
# I personally set enble-renaming to "no" but for now should leave
# at default ("yes") value. 
#
# --enable-renaming=no disables the automatic renaming of every function, etc. 
# to have version prefix. Using it guarantees that you don't accidentally get incorrect
# version of library in case multiple versions are available, which since several
# dependencies use icu, then this is quite possible. Currently I believe they are using icu 
# 67. I have been using ICU 71.1, the latest, but I have tried it with 67 as well.
#
# I find it easier to debug without the renaming. However, most of the benefit comes
# from getting your IDE to resolve the function names to something other than the
# dummy names in urename.h. You don't have to use --enable-renaming=no to get the
# benefit of urename (Need to verify)

# enable-debug=yes is probably not of much value. Simply compiles with debug
# enable-release=no is probably not of much value.

# enable-draft=no is a good idea. Hides draft APIs
# enable-tests=no Disables testing. In order to run tests you have to enable
# several other options that are best not in release. 
# enable-samples=no. Don't need sample code built. Also requires some other
# options that are not best in release
# enable-strict=yes Sounds like a good idea
# with-data-packaging=library. Controls where the icu data is stored.
# This is not the suggested way of doing it. But this is easiest.
#
# Other options exist that look like a good idea but I had some trouble getting
# them all to play together. I can ask ICU if this becomes an issue, such
# as if we run the ICU tests to validate the build.
# 
declare -a ICU_CONFIG_OPTS
#ICU_CONFIG_OPTS+=(--enable-renaming=no)  
#ICU_CONFIG_OPTS+=(--enable-debug=yes)
ICU_CONFIG_OPTS+=(--enable-release=yes)
ICU_CONFIG_OPTS+=(--enable-draft=no)
ICU_CONFIG_OPTS+=(--enable-tests=no)
ICU_CONFIG_OPTS+=(--enable-samples=no)
ICU_CONFIG_OPTS+=(--enable-strict=yes)
ICU_CONFIG_OPTS+=(--with-data-packaging=library)

declare -a ICU_DEFINES

#ICU_DEFINES+=(-DUCONFIG_USE_LOCAL=1)    # include uconfig_local.h
ICU_DEFINES+=(-DU_NO_DEFAULT_INCLUDE_UTF_HEADERS=1)
#ICU_DEFINES+=(-DU_DISABLE_RENAMING=1)
ICU_DEFINES+=(-DU_DEFAULT_SHOW_DRAFT=0)
ICU_DEFINES+=(-DU_HIDE_DRAFT_API=1)
ICU_DEFINES+=(-DU_HIDE_DEPRECATED_API=1)
ICU_DEFINES+=(-DU_HIDE_OBSOLETE_UTF_OLD_H=1)
ICU_DEFINES+=(-DUNISTR_FROM_STRING_EXPLICIT=explicit)  # Can not build test suites with this
ICU_DEFINES+=(-DUNISTR_FROM_CHAR_EXPLICIT=explicit)    # Can not build test suites with this
ICU_DEFINES+=(-DUCONFIG_NO_LEGACY_CONVERSION=1)  # Only support UTF-7/8/16/32, CESU-8, SCSU, BOCU-1, US-ASCII & ISO-8859-1
#
# Default size of UnicodeString. This allows for 27 UTF-16 codeunits
# to be stored in the stack-allocated Unicodestring. Since Kodi
# uses UnicodeString for short-term, it may make sense to pump 
# this up to avoid heap usage.
#
#ICU_DEFINES+=(-DUNISTR_OBJECT_SIZE=64)
ICU_DEFINES+=(-DUNISTR_OBJECT_SIZE=256)

CMAKE_CXX_FLAGS+=" ${ICU_DEFINES[@]} "

TIDY=""

CFLAGS="${C_COMPILER_OPTIONS}"
CXXFLAGS="${C_COMPILER_OPTIONS}"


# Options to build tools
#C_COMPILER_OPTIONS_TOOLS="-O0 -g"
#C_COMPILER_OPTIONS_TOOLS="-O3"

declare -a CMAKE_CXX_FLAGS_TOOLS
CMAKE_CXX_FLAGS_TOOLS+=("${C_COMPILER_OPTIONS_TOOLS}")
CFLAGS_TOOLS="${C_COMPILER_OPTIONS_TOOLS}"
CXXFLAGS_TOOLS="${C_COMPILER_OPTIONS_TOOLS}"


mkdir -p ${BUILD_DIR}
mkdir -p ${BUILD_DIR}/build/include
mkdir -p ${INSTALL_DIR}

cd $KODI_SOURCE

