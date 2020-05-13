# Android packaging

find_program(AAPT_EXECUTABLE aapt PATHS ${SDK_BUILDTOOLS_PATH})
if(NOT AAPT_EXECUTABLE)
  message(FATAL_ERROR "Could NOT find aapt executable")
endif()

find_program(DX_EXECUTABLE dx PATHS ${SDK_BUILDTOOLS_PATH})
if(NOT DX_EXECUTABLE)
  message(FATAL_ERROR "Could NOT find dx executable")
endif()

find_program(ZIPALIGN_EXECUTABLE zipalign PATHS ${SDK_BUILDTOOLS_PATH})
if(NOT ZIPALIGN_EXECUTABLE)
  message(FATAL_ERROR "Could NOT find zipalign executable")
endif()

find_program(ZIP_EXECUTABLE zip)
if(NOT ZIP_EXECUTABLE)
  message(FATAL_ERROR "Could NOT find zip executable")
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(ANDROID_DEBUGGABLE true)
else()
  set(ANDROID_DEBUGGABLE false)
endif()

set(ANDROID_PACKAGING_SOURCE_DIR ${CMAKE_SOURCE_DIR}/tools/android/packaging)
set(ANDROID_PACKAGING_BINARY_DIR ${CMAKE_BINARY_DIR}/tools/android/packaging)

# Configure files into packaging environment.
configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/make_symbols.sh
               ${ANDROID_PACKAGING_BINARY_DIR}/make_symbols.sh COPYONLY)
configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/build.gradle
               ${ANDROID_PACKAGING_BINARY_DIR}/build.gradle COPYONLY)
configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/gradlew
               ${ANDROID_PACKAGING_BINARY_DIR}/gradlew COPYONLY)
configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/settings.gradle
               ${ANDROID_PACKAGING_BINARY_DIR}/settings.gradle COPYONLY)
configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/gradle/wrapper/gradle-wrapper.jar
               ${ANDROID_PACKAGING_BINARY_DIR}/gradle/wrapper/gradle-wrapper.jar COPYONLY)
configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/gradle/wrapper/gradle-wrapper.properties
               ${ANDROID_PACKAGING_BINARY_DIR}/gradle/wrapper/gradle-wrapper.properties COPYONLY)

string(REPLACE "." ";" APP_VERSION_CODE_LIST ${APP_VERSION_CODE})
list(GET APP_VERSION_CODE_LIST 0 major)
list(GET APP_VERSION_CODE_LIST 1 minor)
list(GET APP_VERSION_CODE_LIST 2 patch)
unset(APP_VERSION_CODE_LIST)
math(EXPR APP_VERSION_CODE_ANDROID "(${major} * 100 + ${minor}) * 1000 + ${patch}")
unset(major)
unset(minor)
if(ARCH STREQUAL aarch64 AND patch LESS 999)
  math(EXPR APP_VERSION_CODE_ANDROID "${APP_VERSION_CODE_ANDROID} + 1")
endif()
unset(patch)

set(package_files Splash.java
                  Main.java
                  XBMCBroadcastReceiver.java
                  XBMCInputDeviceListener.java
                  XBMCJsonRPC.java
                  XBMCMainView.java
                  XBMCMediaSession.java
                  XBMCRecommendationBuilder.java
                  XBMCSearchableActivity.java
                  XBMCSettingsContentObserver.java
                  XBMCProperties.java
                  XBMCVideoView.java
                  XBMCFile.java
                  channels/SyncChannelJobService.java
                  channels/SyncProgramsJobService.java
                  channels/model/XBMCDatabase.java
                  channels/model/Subscription.java
                  channels/util/SharedPreferencesHelper.java
                  channels/util/TvUtil.java
                  interfaces/XBMCAudioManagerOnAudioFocusChangeListener.java
                  interfaces/XBMCSurfaceTextureOnFrameAvailableListener.java
                  interfaces/XBMCNsdManagerResolveListener.java
                  interfaces/XBMCNsdManagerRegistrationListener.java
                  interfaces/XBMCNsdManagerDiscoveryListener.java
                  interfaces/XBMCMediaDrmOnEventListener.java
                  interfaces/XBMCDisplayManagerDisplayListener.java
                  model/TVEpisode.java
                  model/Movie.java
                  model/TVShow.java
                  model/File.java
                  model/Album.java
                  model/Song.java
                  model/MusicVideo.java
                  model/Media.java
                  content/XBMCFileContentProvider.java
                  content/XBMCMediaContentProvider.java
                  content/XBMCContentProvider.java
                  content/XBMCYTDLContentProvider.java
                  )
foreach(file IN LISTS package_files)
  configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/xbmc/src/${file}.in
                 ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/java/${file} @ONLY)
endforeach()

# Copy files to the location expected by the Android packaging scripts.
add_custom_target(bundle
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${ANDROID_PACKAGING_SOURCE_DIR}/xbmc/res
                                               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/res
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEPENDS_PATH}/lib/python${PYTHON_VERSION} ${libdir}/python${PYTHON_VERSION}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEPENDS_PATH}/share/${APP_NAME_LC} ${datadir}/${APP_NAME_LC}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APP_NAME_LC}>
                                     ${libdir}/${APP_NAME_LC}/$<TARGET_FILE_NAME:${APP_NAME_LC}>)
add_dependencies(bundle ${APP_NAME_LC})

# This function is used to prepare a prefix expected by the Android packaging
# scripts. It creates a bundle_files command that is added to the bundle target.
function(add_bundle_file file destination relative)
  if(NOT TARGET bundle_files)
    file(REMOVE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake)
    add_custom_target(bundle_files COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake)
    add_dependencies(bundle bundle_files)
  endif()

  string(REPLACE "${relative}/" "" outfile ${file})
  get_filename_component(file ${file} REALPATH)
  get_filename_component(outdir ${outfile} DIRECTORY)
  file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
       "file(COPY \"${file}\" DESTINATION \"${destination}/${outdir}\")\n")
  if(file MATCHES "\\.so\\..+$")
    get_filename_component(srcfile "${file}" NAME)
    string(REGEX REPLACE "\\.so\\..+$" "\.so" destfile ${srcfile})
    file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
         "file(RENAME \"${destination}/${outdir}/${srcfile}\" \"${destination}/${outdir}/${destfile}\")\n")
  endif()
endfunction()

# Copy files into prefix
foreach(file IN LISTS XBT_FILES install_data)
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" file ${file})
  add_bundle_file(${CMAKE_BINARY_DIR}/${file} ${datarootdir}/${APP_NAME_LC} ${CMAKE_BINARY_DIR})
endforeach()

foreach(library IN LISTS LIBRARY_FILES)
  add_bundle_file(${library} ${libdir}/${APP_NAME_LC} ${CMAKE_BINARY_DIR})
endforeach()

foreach(lib IN LISTS required_dyload dyload_optional ITEMS Shairplay)
  string(TOUPPER ${lib} lib_up)
  set(lib_so ${${lib_up}_SONAME})
  if(lib_so AND EXISTS ${DEPENDS_PATH}/lib/${lib_so})
    add_bundle_file(${DEPENDS_PATH}/lib/${lib_so} ${libdir} "")
  endif()
endforeach()
add_bundle_file(${ASS_LIBRARY} ${libdir} "")
add_bundle_file(${SHAIRPLAY_LIBRARY} ${libdir} "")
add_bundle_file(${SMBCLIENT_LIBRARY} ${libdir} "")

if(CPU MATCHES "x86_64")
  set(NDK_CPU x86_64)
elseif(CPU MATCHES "i686")
  set(CPU x86)
  set(NDK_CPU x86)
elseif(CPU MATCHES "arm64")
  set(NDK_CPU arm64)
elseif(CPU MATCHES "arm")
  set(NDK_CPU arm)
endif()

# privacy policy
configure_file(${CMAKE_SOURCE_DIR}/privacy-policy.txt
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/assets/privacy-policy.txt COPYONLY)

# splash and icons
configure_file(${CMAKE_SOURCE_DIR}/media/splash.jpg
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/res/drawable/splash.jpg COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/media/splash.jpg
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/res/drawable-xxxhdpi/splash.jpg COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/media/icon80x80.png
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/res/drawable/ic_recommendation_80dp.png COPYONLY)

# other android packaging
configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/xbmc/xbmc.properties.in
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/res/raw/xbmc.properties @ONLY)

configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/xbmc/AndroidManifest.xml.in
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/AndroidManifest.xml @ONLY)

configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/xbmc/build.gradle.in
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/build.gradle @ONLY)

configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/xbmc/strings.xml.in
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/res/values/strings.xml @ONLY)

configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/xbmc/colors.xml.in
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/res/values/colors.xml @ONLY)

configure_file(${ANDROID_PACKAGING_SOURCE_DIR}/xbmc/searchable.xml.in
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/res/xml/searchable.xml @ONLY)

configure_file(${TOOLCHAIN}/sysroot/usr/lib/${HOST}/libc++_shared.so
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/lib/${CPU}/libc++_shared.so COPYONLY)

configure_file(${NDKROOT}/prebuilt/android-${NDK_CPU}/gdbserver/gdbserver
               ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/lib/${CPU}/gdbserver COPYONLY)

# todo: gdb setup?

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LC)

add_custom_command(OUTPUT ${ANDROID_PACKAGING_BINARY_DIR}/.copy-libs-done
                   COMMAND find ${CMAKE_INSTALL_PREFIX}/ -name \"*.so\" -exec cp -fp {} ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/lib/${CPU}/ \\\;
                   COMMAND find ${DEPENDS_PATH}/lib/${APP_NAME_LC}/addons -name \"*.so\" -exec cp -fp {} ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/lib/${CPU}/ \\\;
                   COMMAND ${CMAKE_COMMAND} -E touch ${ANDROID_PACKAGING_BINARY_DIR}/.copy-libs-done
                   DEPENDS bundle
                   COMMENT "Copying libs for packaging")

# hack: I don't like this step and I'm not sure why it's needed however gradle doesn't include
# files in the libs/ directory that don't start with lib (maybe a gradle bug?)
add_custom_command(OUTPUT ${ANDROID_PACKAGING_BINARY_DIR}/.rename-libs-done
                   COMMAND ls *.so | grep -v "^lib" | xargs -I {} mv {} lib{}
                   COMMAND ${CMAKE_COMMAND} -E touch ${ANDROID_PACKAGING_BINARY_DIR}/.rename-libs-done
                   WORKING_DIRECTORY ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/lib/${CPU}/
                   DEPENDS ${ANDROID_PACKAGING_BINARY_DIR}/.copy-libs-done
                   COMMENT "Renaming libs for packaging")

add_custom_command(OUTPUT ${ANDROID_PACKAGING_BINARY_DIR}/.copy-assets-done
                   COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/assets/python${PYTHON_VERSION}/lib
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_VERSION} ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/assets/python${PYTHON_VERSION}/lib/python${PYTHON_VERSION}
                   COMMAND cp -a ${CMAKE_INSTALL_PREFIX}/share/${APP_NAME_LC}/* ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/assets/
                   COMMAND ${CMAKE_COMMAND} -E touch ${ANDROID_PACKAGING_BINARY_DIR}/.copy-assets-done
                   DEPENDS bundle
                   COMMENT "Copying assets for packaging")

add_custom_command(OUTPUT ${ANDROID_PACKAGING_BINARY_DIR}/.remove-items-done
                   COMMAND ${CMAKE_COMMAND} -E remove_directory python${PYTHON_VERSION}/lib/python${PYTHON_VERSION}/test
                   COMMAND ${CMAKE_COMMAND} -E remove_directory python${PYTHON_VERSION}/lib/python${PYTHON_VERSION}/config
                   COMMAND ${CMAKE_COMMAND} -E remove_directory python${PYTHON_VERSION}/lib/python${PYTHON_VERSION}/lib-dynload
                   COMMAND find . -name \"*.so\" -exec rm -rf {} \\\;
                   COMMAND ${CMAKE_COMMAND} -E touch ${ANDROID_PACKAGING_BINARY_DIR}/.remove-items-done
                   WORKING_DIRECTORY ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/assets
                   DEPENDS ${ANDROID_PACKAGING_BINARY_DIR}/.copy-assets-done
                   COMMENT "Removing items for packaging")

add_custom_command(OUTPUT ${ANDROID_PACKAGING_BINARY_DIR}/main.${APP_NAME_LC}.obb
                   COMMAND ${ZIP_EXECUTABLE} -9 -q -r ${ANDROID_PACKAGING_BINARY_DIR}/main.${APP_NAME_LC}.obb assets
                   WORKING_DIRECTORY ${ANDROID_PACKAGING_BINARY_DIR}/xbmc
                   DEPENDS ${ANDROID_PACKAGING_BINARY_DIR}/.remove-items-done
                   COMMENT "Creating main.${APP_NAME_LC}.obb")

add_custom_target(obb
                  COMMAND ${CMAKE_COMMAND} -E copy ${ANDROID_PACKAGING_BINARY_DIR}/main.${APP_NAME_LC}.obb ${CMAKE_SOURCE_DIR}
                  DEPENDS ${ANDROID_PACKAGING_BINARY_DIR}/main.${APP_NAME_LC}.obb)

add_custom_command(OUTPUT ${ANDROID_PACKAGING_BINARY_DIR}/.strip-libs-done
                   COMMAND ${CMAKE_STRIP} --strip-unneeded ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/lib/${CPU}/*.so
                   COMMAND ${CMAKE_COMMAND} -E touch ${ANDROID_PACKAGING_BINARY_DIR}/.strip-libs-done
                   DEPENDS ${ANDROID_PACKAGING_BINARY_DIR}/.rename-libs-done
                   COMMENT "Stripping libs for packaging")

if(NOT DEFINED ENV{KODI_ANDROID_KEY_ALIAS})
  list(APPEND ANDROID_KEY_ENV KODI_ANDROID_KEY_ALIAS=androiddebugkey)
endif()

if(NOT DEFINED ENV{KODI_ANDROID_KEY_PASSWORD})
  list(APPEND ANDROID_KEY_ENV KODI_ANDROID_KEY_PASSWORD=android)
endif()

if(NOT DEFINED ENV{KODI_ANDROID_STORE_FILE})
  list(APPEND ANDROID_KEY_ENV KODI_ANDROID_STORE_FILE=$ENV{HOME}/.android/debug.keystore)
  set(ENV{KODI_ANDROID_STORE_FILE} $ENV{HOME}/.android/debug.keystore)
endif()

if(NOT EXISTS $ENV{KODI_ANDROID_STORE_FILE})
  message(FATAL_ERROR "Android keystore doesn't exist at $ENV{KODI_ANDROID_STORE_FILE} Create it or set the ENV variable KODI_ANDROID_STORE_FILE to point to a different keystore")
endif()

if(NOT DEFINED ENV{KODI_ANDROID_STORE_PASSWORD})
  list(APPEND ANDROID_KEY_ENV KODI_ANDROID_STORE_PASSWORD=android)
endif()

add_custom_command(OUTPUT ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/build/outputs/apk/${CMAKE_BUILD_TYPE_LC}/${APP_NAME_LC}app-${CPU}-${CMAKE_BUILD_TYPE_LC}.apk
                   COMMAND env ANDROID_HOME=${SDKROOT} ${ANDROID_KEY_ENV} ./gradlew clean assemble${CMAKE_BUILD_TYPE}
                   COMMAND ${ZIPALIGN_EXECUTABLE} -c 4 xbmc/build/outputs/apk/${CMAKE_BUILD_TYPE_LC}/${APP_NAME_LC}app-${CPU}-${CMAKE_BUILD_TYPE_LC}.apk
                   WORKING_DIRECTORY ${ANDROID_PACKAGING_BINARY_DIR}
                   DEPENDS ${ANDROID_PACKAGING_BINARY_DIR}/.strip-libs-done
                           ${ANDROID_PACKAGING_BINARY_DIR}/.remove-items-done
                   COMMENT "Running Gradle Build")

add_custom_target(apk
                  COMMAND ${CMAKE_COMMAND} -E copy ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/build/outputs/apk/${CMAKE_BUILD_TYPE_LC}/${APP_NAME_LC}app-${CPU}-${CMAKE_BUILD_TYPE_LC}.apk ${CMAKE_SOURCE_DIR}/
                  DEPENDS ${ANDROID_PACKAGING_BINARY_DIR}/xbmc/build/outputs/apk/${CMAKE_BUILD_TYPE_LC}/${APP_NAME_LC}app-${CPU}-${CMAKE_BUILD_TYPE_LC}.apk)

add_custom_target(apk-obb
                  DEPENDS apk obb)
