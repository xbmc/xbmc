# Android packaging

# Configure variables used in replacement configure_file calls
if(CPU MATCHES i686)
  set(CPU x86)
endif()

string(REPLACE "." ";" APP_VERSION_CODE_LIST ${APP_VERSION_CODE})
list(GET APP_VERSION_CODE_LIST 0 major)
list(GET APP_VERSION_CODE_LIST 1 minor)
list(GET APP_VERSION_CODE_LIST 2 patch)
unset(APP_VERSION_CODE_LIST)
math(EXPR APP_VERSION_CODE_ANDROID "(${major} * 100 + ${minor}) * 1000 + ${patch}")
unset(major)
unset(minor)
unset(patch)

string(REPLACE "." "/" dot_APP_PACKAGE ${APP_PACKAGE})

# Configure files into packaging environment.
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/Makefile.in
               ${CMAKE_BINARY_DIR}/tools/android/packaging/Makefile @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/gradle.properties
               ${CMAKE_BINARY_DIR}/tools/android/packaging/gradle.properties COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/build.gradle
               ${CMAKE_BINARY_DIR}/tools/android/packaging/build.gradle COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/gradlew
               ${CMAKE_BINARY_DIR}/tools/android/packaging/gradlew COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/settings.gradle
               ${CMAKE_BINARY_DIR}/tools/android/packaging/settings.gradle COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/gradle/wrapper/gradle-wrapper.jar
               ${CMAKE_BINARY_DIR}/tools/android/packaging/gradle/wrapper/gradle-wrapper.jar COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/gradle/wrapper/gradle-wrapper.properties
               ${CMAKE_BINARY_DIR}/tools/android/packaging/gradle/wrapper/gradle-wrapper.properties COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/jni/Android.mk
               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/jni/Android.mk COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/strings.xml.in
               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/res/values/strings.xml @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/colors.xml.in
               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/res/values/colors.xml @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/searchable.xml.in
               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/res/xml/searchable.xml @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/AndroidManifest.xml.in
               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/AndroidManifest.xml @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/build.gradle.in
               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/build.gradle @ONLY)

set(src_java_files Splash.java
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
                   XBMCTextureCache.java
                   XBMCURIUtils.java
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
                   interfaces/XBMCMediaDrmOnKeyStatusChangeListener.java
                   interfaces/XBMCDisplayManagerDisplayListener.java
                   interfaces/XBMCSpeechRecognitionListener.java
                   interfaces/XBMCConnectivityManagerNetworkCallback.java
                   model/TVEpisode.java
                   model/Movie.java
                   model/TVShow.java
                   model/File.java
                   model/Album.java
                   model/Song.java
                   model/MusicVideo.java
                   model/Media.java
                   content/XBMCFileContentProvider.java
                   content/XBMCFileProvider.java
                   content/XBMCMediaContentProvider.java
                   content/XBMCContentProvider.java
                   util/Storage.java
                  )

foreach(file IN LISTS src_java_files)
  configure_file(${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/src/${file}.in
                 ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/java/${dot_APP_PACKAGE}/${file} @ONLY)
endforeach()

# Copies dir to destination, excluding any shared libs
function(add_bundle_dir dir dir_dest)
  file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
              "file(COPY \"${dir}/\"\n"
              "     DESTINATION \"${dir_dest}/\"\n"
              "     REGEX \".*\\\\.so$\" EXCLUDE)\n")
endfunction()

# Copies library to destination, renaming to android required lib prefix if needed
function(add_bundle_lib input destination)
  if(TARGET ${input})
    # TARGET is required to have IMPORTED_LOCATION
    get_target_property(imploc_file ${input} IMPORTED_LOCATION)

    if(imploc_file)
      set(file ${imploc_file})
    else()
      return()
    endif()
  else()
    set(file ${input})
  endif()

  cmake_path(GET file FILENAME out_file)

  # Check if we need to add lib prefix
  string(FIND ${out_file} "lib" lib_prefix)
  if(${lib_prefix} EQUAL "-1")
    set(_prefix lib)
  endif()

  file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
       "configure_file(\"${file}\" \"${destination}/${_prefix}${out_file}\" COPYONLY)\n")
endfunction()

# Copy app shared lib to destination packaging
add_custom_target(bundle
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APP_NAME_LC}>
                                     ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/lib/${CPU}
    COMMAND ${CMAKE_STRIP} --strip-unneeded ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/lib/${CPU}/*.so)

add_dependencies(bundle ${APP_NAME_LC})

if(NOT TARGET bundle_files)
  file(REMOVE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake)
  add_custom_target(bundle_files COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake)
  add_dependencies(bundle bundle_files)
  add_dependencies(bundle_files ${APP_NAME_LC})
endif()

add_bundle_dir("${CMAKE_SOURCE_DIR}/tools/android/packaging/xbmc/res" "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/res")

# This copies extracted pil to addons folder prior to bundle step for addons
# This runs at configure time, if python modules are ever built at build time, this will need
# to be revisited
file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
            "file(COPY \"${DEPENDS_PATH}/share/${APP_NAME_LC}/addons/\"\n"
            "     DESTINATION \"${CMAKE_BINARY_DIR}/addons\"\n"
            "     REGEX \".*\\\\.so$\" EXCLUDE)\n")

# Copy addon shared lib files to package lib folder
file(GLOB_RECURSE _addon_libs CONFIGURE_DEPENDS ${DEPENDS_PATH}/share/${APP_NAME_LC}/addons/*.so)

foreach(file IN LISTS _addon_libs)
  add_bundle_lib(${file} "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/lib/${CPU}")
endforeach()

# libc++
add_bundle_lib("${TOOLCHAIN}/sysroot/usr/lib/${HOST}/libc++_shared.so" "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/lib/${CPU}/")

configure_file("${CMAKE_SOURCE_DIR}/privacy-policy.txt" "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/assets/privacy-policy.txt" COPYONLY)

# Copy res data to package folder
configure_file(${CMAKE_SOURCE_DIR}/media/applaunch_screen.png
               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/res/drawable-xxxhdpi/applaunch_screen.png COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/media/applaunch_screen.png
               ${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/res/drawable/applaunch_screen.png COPYONLY)

file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
            "file(COPY \"${CMAKE_SOURCE_DIR}/tools/android/packaging/media/\"\n"
            "     DESTINATION \"${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/res\"\n"
            "     PATTERN \"mipmap-*\"\n"
            "     PATTERN playstore.png EXCLUDE)\n")

# Copy files into package assets folder
add_bundle_dir("${CMAKE_BINARY_DIR}/addons" "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/assets/addons")
add_bundle_dir("${CMAKE_BINARY_DIR}/media" "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/assets/media")
add_bundle_dir("${CMAKE_BINARY_DIR}/system" "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/assets/system")
add_bundle_dir("${CMAKE_BINARY_DIR}/userdata" "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/assets/userdata")

# Copy python folder structure, excluding any shared archives
# PYTHONHOME use requires the structure <PYTHONHOME>/lib/python${PYTHON_VERSION}
file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
            "file(COPY ${DEPENDS_PATH}/lib/python${PYTHON_VERSION}/\n"
            "     DESTINATION \"${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/assets/python${PYTHON_VERSION}/lib/python${PYTHON_VERSION}/\"\n"
            "     PATTERN \"lib-dynload\" EXCLUDE\n"
            "     PATTERN \"config-${PYTHON_VERSION}\" EXCLUDE\n"
            "     REGEX \".*\\\\.so$\" EXCLUDE)\n")

# Copy python shared lib files to package lib folder
file(GLOB_RECURSE _py_libs CONFIGURE_DEPENDS ${DEPENDS_PATH}/lib/python${PYTHON_VERSION}/*.so)

file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
            "file(COPY ${_py_libs}\n"
            "     DESTINATION \"${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/lib/${CPU}/\"\n"
            "     FILES_MATCHING PATTERN \"*.so\"\n"
            "     REGEX \".*[Cc]ryptodome+.*\" EXCLUDE)\n")

# Cryptodome is special and requires further renaming due to runtime patches
list(FILTER _py_libs INCLUDE REGEX ".*[Cc]ryptodome+.*")

foreach(file IN LISTS _py_libs)
  block()
    string(REGEX REPLACE "\.abi[0-9]" "" str_abistrip ${file})

    string(REGEX MATCH "[Cc]ryptodome.+\.so" str_pathname ${str_abistrip})
    string(REPLACE "/" "_" out_file ${str_pathname})

    # Check if we need to add lib prefix
    string(FIND ${out_file} "lib" lib_prefix)
    if(${lib_prefix} EQUAL "-1")
      set(_prefix lib)
    endif()

    file(APPEND ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/BundleFiles.cmake
                "configure_file(\"${file}\" \"${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/lib/${CPU}/${_prefix}${out_file}\" COPYONLY)\n")
  endblock()
endforeach()

if(TARGET ${APP_NAME_LC}::LibDvd)
  add_bundle_lib(${APP_NAME_LC}::LibDvd "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/lib/${CPU}")
endif()

if(TARGET ${APP_NAME_LC}::Shairplay)
  add_bundle_lib(${APP_NAME_LC}::Shairplay "${CMAKE_BINARY_DIR}/tools/android/packaging/xbmc/lib/${CPU}")
endif()

find_program(MAKE_EXECUTABLE make REQUIRED)

add_custom_target(apk
    COMMAND env PATH=${NATIVEPREFIX}/bin:$ENV{PATH} ${MAKE_EXECUTABLE} -j1
            -C ${CMAKE_BINARY_DIR}/tools/android/packaging
            apk
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tools/android/packaging
    VERBATIM
)

add_dependencies(apk bundle)
