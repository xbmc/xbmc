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

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(ANDROID_DEBUGGABLE true)
else()
  set(ANDROID_DEBUGGABLE false)
endif()

# Configure files into packaging environment.
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/Makefile.in
               ${CMAKE_BINARY_DIR}/tools/android/packaging/Makefile @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/apksign
               ${CMAKE_BINARY_DIR}/tools/android/packaging/apksign COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/make_symbols.sh
               ${CMAKE_BINARY_DIR}/tools/android/packaging/make_symbols.sh COPYONLY)
file(WRITE ${CMAKE_BINARY_DIR}/tools/depends/Makefile.include
     "$(PREFIX)/lib/${APP_NAME_LC}/lib${APP_NAME_LC}.so: ;\n")

set(package_files strings.xml
                  activity_main.xml
                  colors.xml
                  searchable.xml
                  AndroidManifest.xml
                  src/org/xbmc/kodi/XBMCOnAudioFocusChangeListener.java
                  src/org/xbmc/kodi/XBMCInputDeviceListener.java
                  src/org/xbmc/kodi/Main.java
                  src/org/xbmc/kodi/XBMCSettingsContentObserver.java
                  src/org/xbmc/kodi/XBMCOnFrameAvailableListener.java
                  src/org/xbmc/kodi/XBMCProperties.java
                  src/org/xbmc/kodi/XBMCVideoView.java
                  src/org/xbmc/kodi/Splash.java
                  src/org/xbmc/kodi/XBMCBroadcastReceiver.java)
foreach(file IN LISTS package_files)
  configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/${file}.in
                 ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/${file} @ONLY)
endforeach()

# Copy files to the location expected by the Android packaging scripts.
add_custom_target(bundle
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tools/android/packaging/media
                                               ${CMAKE_BINARY_DIR}/tools/android/packaging/media
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/res
                                               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/res
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEPENDS_PATH}/lib/python2.7 ${libdir}/python2.7
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
add_bundle_file(${SMBCLIENT_LIBRARY} ${libdir} "")

# Main targets from Makefile.in
if(CPU MATCHES i686)
  set(CPU x86)
  set(ARCH x86)
endif()
foreach(target apk obb apk-unsigned apk-obb apk-obb-unsigned apk-noobb apk-clean apk-sign)
  add_custom_target(${target}
      COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} ${CMAKE_MAKE_PROGRAM}
              -C ${CMAKE_BINARY_DIR}/tools/android/packaging
              CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
              CC=${CMAKE_C_COMPILER}
              CPU=${CPU}
              ARCH=${ARCH}
              PREFIX=${prefix}
              NDKROOT=${NDKROOT}
              SDKROOT=${SDKROOT}
              SDK_PLATFORM=${SDK_PLATFORM}
              STRIP=${CMAKE_STRIP}
              AAPT=${AAPT_EXECUTABLE}
              DX=${DX_EXECUTABLE}
              ZIPALIGN=${ZIPALIGN_EXECUTABLE}
              ${target}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tools/android/packaging
  )
  if(NOT target STREQUAL apk-clean)
    add_dependencies(${target} bundle)
  endif()
endforeach()
