
include_directories(
	${root}/xbmc/cores
	${root}/xbmc/guilib
	${root}/xbmc/network
	${root}/xbmc/music
	${root}/xbmc/filesystem
	${root}/xbmc/threads
	${root}/xbmc/video
	${root}/xbmc/music/tags
	${root}/xbmc/dialogs
	${root}/xbmc/pictures
	${root}/xbmc/playlists
	${root}/plex/FileSystem
	${root}/plex/Network
  ${CONFIG_INCLUDE_PATH_SPARKLE}
)

add_subdirectory(Client)
add_subdirectory(FileSystem)
add_subdirectory(GUI)
add_subdirectory(Network)
add_subdirectory(Owned)
add_subdirectory(Players)
add_subdirectory(Utility)
add_subdirectory(AutoUpdate)

if(TARGET_WIN32)
  add_subdirectory(Resources/MCE)
endif(TARGET_WIN32)

if(TARGET_OSX)
  add_subdirectory(Helper)
endif(TARGET_OSX)

find_all_sources(. plex_SRCS)

list(REMOVE_ITEM plex_SRCS ./pch_plex.cpp)
list(APPEND plex_SRCS Resources/Plex.rc)
list(APPEND plex_SRCS Resources/Plex.rc2)

set(Headers PlexTypes.h)
if(NOT TARGET_OSX)
  list(REMOVE_ITEM plex_SRCS ./PlexMacUtils.mm)
endif(NOT TARGET_OSX)

if(NOT TARGET_WIN32)
  list(REMOVE_ITEM plex_SRCS ./MediaCenterLaunchHost.cpp)
endif(NOT TARGET_WIN32)

get_property(PLEX_MODULE_SRCS GLOBAL PROPERTY SRCS_LIST)
set(PLEX_ALL_SRCS ${PLEX_MODULE_SRCS} ${plex_SRCS} ${Headers})

ADD_MSVC_PRECOMPILED_HEADER("pch_plex.h" "pch_plex.cpp" PLEX_ALL_SRCS)

if(TARGET_COMMON_DARWIN)
  add_executable(${PLEX_TARGET_NAME} MACOSX_BUNDLE ${PLEX_ALL_SRCS})
  set_target_properties(${PLEX_TARGET_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${plexdir}/Resources/Info.plist.in")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PLEX_VERSION_STRING}")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PLEX_VERSION_STRING_SHORT}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${PLEX_VERSION_STRING}")
else()
  add_executable(${PLEX_TARGET_NAME} WIN32 ${PLEX_ALL_SRCS})
endif()

# Set the correct binary name
set_target_properties(${PLEX_TARGET_NAME} PROPERTIES OUTPUT_NAME ${EXECUTABLE_NAME})

add_dependencies(${PLEX_TARGET_NAME} git_revision.h)
target_link_libraries(${PLEX_TARGET_NAME} xbmc pcre ${CONFIG_INTERNAL_LIBS} ${CONFIG_PLEX_LINK_LIBRARIES})

if(ENABLE_PYTHON)
  target_link_libraries(${PLEX_TARGET_NAME} addons_python)
endif(ENABLE_PYTHON)


# find all image files in the skin
file(GLOB MEDIA_IMAGES_MEDIASTREAM ${root}/addons/skin.mediastream/Media/*.png ${root}/addons/skin.mediastream/Media/*.gif)
file(GLOB MEDIA_IMAGES_PLEX_SKIN ${root}/addons/skin.plex/Media/*.png ${root}/addons/skin.plex/Media/*.gif)

if(COMPRESS_TEXTURES)
  # Build the packed textures
  if(DEFINED USING_XCODE)
    set(TEXTURE_PACKER_BIN ${CMAKE_BINARY_DIR}/tools/TexturePacker/Debug/TexturePacker)
  else()
    set(TEXTURE_PACKER_BIN ${CMAKE_BINARY_DIR}/tools/TexturePacker/TexturePacker)
  endif()

  if(WIN32)
    set(WORKDIR ${root}/tools/TexturePacker)
  else(WIN32)
    set(WORKDIR ${CMAKE_CURRENT_BINARY_DIR})
  endif(WIN32)

  add_custom_command(
    OUTPUT Textures_MediaStream.xbt
    COMMAND ${TEXTURE_PACKER_BIN} -input ${root}/addons/skin.mediastream/Media -output ${CMAKE_CURRENT_BINARY_DIR}/Textures_MediaStream.xbt
    MAIN_DEPENDENCY ${MEDIA_IMAGES_MEDIASTREAM}
    DEPENDS TexturePacker
    WORKING_DIRECTORY ${WORKDIR}
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Textures_MediaStream.xbt DESTINATION ${RESOURCEPATH}/addons/skin.mediastream/Media RENAME Textures.xbt COMPONENT RUNTIME)

  if(ENABLE_NEW_SKIN)
    add_custom_command(
      OUTPUT Textures_Plex.xbt
      COMMAND ${TEXTURE_PACKER_BIN} -input ${root}/addons/skin.plex/Media -output Textures_Plex.xbt
      MAIN_DEPENDENCY ${MEDIA_IMAGES_PLEX_SKIN}
      DEPENDS TexturePacker
    )
    add_custom_target(CompressTextures ALL DEPENDS Textures_MediaStream.xbt Textures_Plex.xbt)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Textures_Plex.xbt DESTINATION ${RESOURCEPATH}/addons/skin.plex/Media RENAME Textures.xbt COMPONENT RUNTIME)
  else(ENABLE_NEW_SKIN)
    add_custom_target(CompressTextures ALL DEPENDS Textures_MediaStream.xbt)
  endif(ENABLE_NEW_SKIN)

  set(EXCLUDE_TEXTURES "skin.(mediastream|plex)/Media/*")
else()
  set(EXCLUDE_TEXTURES ^foo)
endif()

# Set some variables we need to configure the CMakeCompleteBundle file
set(PLEXDIR ${plexdir})
set(ROOTDIR ${root})
set(DEPENDDIR ${dependdir})
configure_file(CMakeCompleteBundle.cmake.in CMakeCompleteBundle.cmake @ONLY)

if(TARGET_COMMON_DARWIN OR TARGET_WIN32)
  install(FILES ${CONFIG_PLEX_INSTALL_LIBRARIES} DESTINATION ${LIBPATH} COMPONENT RUNTIME)
endif()

if(TARGET_COMMON_DARWIN)
  install(TARGETS ${PLEX_TARGET_NAME} BUNDLE DESTINATION . COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Plex.icns DESTINATION ${EXECUTABLE_NAME}.app/Contents/Resources COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Credits.html DESTINATION ${EXECUTABLE_NAME}.app/Contents/Resources COMPONENT RUNTIME)
else(TARGET_COMMON_DARWIN)
  install(TARGETS ${PLEX_TARGET_NAME} RUNTIME DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Credits.html DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
endif(TARGET_COMMON_DARWIN)

install(DIRECTORY ${root}/media ${root}/sounds ${root}/language DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
        PATTERN Splash.png EXCLUDE
        PATTERN Credits.html EXCLUDE)

install(DIRECTORY ${root}/system DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
        PATTERN python EXCLUDE
        PATTERN peripherals.xml EXCLUDE)

if(NOT ENABLE_NEW_SKIN)
  set(EXCLUDE_NEW_SKIN skin.plex)
else(NOT ENABLE_NEW_SKIN)
  set(EXCLUDE_NEW_SKIN ^foo)
endif(NOT ENABLE_NEW_SKIN)

install(DIRECTORY ${root}/addons DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME
        PATTERN skin.confluence EXCLUDE
        PATTERN skin.touched EXCLUDE
        PATTERN ${EXCLUDE_NEW_SKIN} EXCLUDE
        REGEX screensaver.rsxs* EXCLUDE
        REGEX library.* EXCLUDE
        REGEX metadata.* EXCLUDE
        REGEX weather.* EXCLUDE
        PATTERN repository.xbmc.org EXCLUDE
        REGEX ${EXCLUDE_TEXTURES} EXCLUDE
        PATTERN .git EXCLUDE
        PATTERN xbmc.python EXCLUDE
)

if(ENABLE_PYTHON AND TARGET_OSX)
  install(DIRECTORY ${dependdir}/lib/python2.6 DESTINATION ${LIBPATH}/lib/ COMPONENT RUNTIME 
    PATTERN python2.6/test EXCLUDE
    REGEX .*\\.pyc$ EXCLUDE
    REGEX .*\\.pyo$ EXCLUDE
  )
  install(FILES ${dependdir}/include/python2.6/pyconfig.h DESTINATION ${LIBPATH}/include/python2.6 COMPONENT RUNTIME)
endif()

if(NOT DEFINED PLEX_SPLASH)
  if(TARGET_RPI)
    set(PLEX_SPLASH Splash-RPI.png)
  else(TARGET_RPI)
    set(PLEX_SPLASH Splash.png)
  endif(TARGET_RPI)
endif(NOT DEFINED PLEX_SPLASH)

install(FILES ${plexdir}/Resources/${PLEX_SPLASH} DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME RENAME Splash.png)

install(DIRECTORY ${plexdir}/addons DESTINATION DESTINATION ${RESOURCEPATH} COMPONENT RUNTIME)
install(FILES ${plexdir}/Resources/plex-icon-120.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
install(FILES ${plexdir}/Resources/plex-icon-256.png DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
install(FILES ${plexdir}/Resources/peripherals.xml DESTINATION ${RESOURCEPATH}/system COMPONENT RUNTIME)

if(TARGET_OSX)
  install(FILES ${plexdir}/Resources/com.plexapp.ht.helper.plist DESTINATION ${RESOURCEPATH}/tools/darwin/runtime COMPONENT RUNTIME)
elseif(TARGET_WIN32)
  install(FILES ${root}/system/zlib1.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${root}/project/Win32BuildSetup/dependencies/libcdio-13.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/build/dependencies/vcredist/2010/vcredist_x86.exe DESTINATION ${BINPATH}/Dependencies COMPONENT VCREDIST)
  install(DIRECTORY ${plexdir}/build/dependencies/dxsetup DESTINATION ${BINPATH}/Dependencies COMPONENT QDXSETUP)
  install(FILES ${root}/project/Win32BuildSetup/dependencies/glew32.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${root}/project/Win32BuildSetup/dependencies/libiconv-2.dll DESTINATION ${BINPATH} COMPONENT RUNTIME)
  install(FILES ${plexdir}/Resources/Plex.ico ${plexdir}/Resources/PlexBanner.bmp DESTINATION ${RESOURCEPATH}/media COMPONENT RUNTIME)
endif()

if(TARGET_OSX)
  set(DSYM_FNAME "${EXECUTABLE_NAME}-${PLEX_VERSION_STRING}-${OSX_ARCH}.dSYM")
  add_custom_command(
    OUTPUT "${DSYM_FNAME}"
    COMMAND dsymutil -o "${CMAKE_CURRENT_BINARY_DIR}/${DSYM_FNAME}" "${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE_NAME}.app/Contents/MacOS/${EXECUTABLE_NAME}"
    DEPENDS ${PLEX_TARGET_NAME}
  )
  add_custom_command(
    OUTPUT "${DSYM_FNAME}.zip"
    COMMAND ditto -c -k "${CMAKE_CURRENT_BINARY_DIR}/${DSYM_FNAME}" "${CMAKE_CURRENT_BINARY_DIR}/${DSYM_FNAME}.zip"
    DEPENDS "${DSYM_FNAME}"
  )
  add_custom_target(Symbols DEPENDS "${DSYM_FNAME}.zip")
endif()

if(CREATE_BUNDLE)
  if(TARGET_COMMON_DARWIN OR TARGET_WIN32)
    install(SCRIPT ${CMAKE_BINARY_DIR}/plex/CMakeCompleteBundle.cmake COMPONENT RUNTIME)
  endif()
  if(TARGET_OSX)
    add_custom_target(BundlingSymbol ALL DEPENDS Symbols)
  endif()
endif()
